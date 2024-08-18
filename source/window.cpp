#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "window_manager.h"
#include "window.h"
#include "config.h"
#include "eshywm.h"
#include "button.h"
#include "switcher.h"
#include "util.h"
#include "X11.h"
#include "image.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <span>
#include <assert.h>

constexpr int half_of(const int x)
{
    return x / 2;
}

EshyWMWindow::EshyWMWindow(Window _window)
    : window(_window)
    , parent_workspace(nullptr)
    , b_show_titlebar(false)
    , window_icon(nullptr)
    , window_font(nullptr)
    , frame_geometry({})
    , pre_state_change_geometry({})
    , window_state(WS_NONE)
    , close_button(nullptr)
{
    window_icon = X11::retrieve_window_icon(window);
    if (!window_icon)
        window_icon = new Image(EshyWMConfig::default_application_image_path.c_str());
    
    imlib_add_path_to_font_path("/usr/share/fonts/TTF");
    window_font = imlib_load_font("Lato-Regular/14");
}

EshyWMWindow::~EshyWMWindow()
{
    delete close_button;
    delete window_icon;
}

void EshyWMWindow::initialize(const X11::WindowAttributes& attributes)
{
    //Center window on the output the cursor is in
    const auto [cursor_x, cursor_y] = X11::get_cursor_position();
    auto output = output_at_position(cursor_x, cursor_y);
    assert(output && output->active_workspace);
    parent_workspace = output->active_workspace;

    Rect window_geometry;
    window_geometry.width = std::min<uint>(output->geometry.width * 0.9f, attributes.width);
    window_geometry.height = std::min<uint>(output->geometry.height * 0.9f, attributes.height);
    window_geometry.x = std::clamp(center_x(output, window_geometry.width), output->geometry.x, center_x(output, 0));
    window_geometry.y = std::clamp(center_y(output, window_geometry.height), output->geometry.y, center_y(output, 0));

    frame_geometry = window_geometry;
    pre_state_change_geometry = window_geometry;

    //Resize because the bottom is cut off when we use the * 0.9 version of height
    X11::resize_window(window, window_geometry);
    X11::set_input_masks(window, PointerMotionMask | StructureNotifyMask | PropertyChangeMask);

    //If window was previously maximized when it was closed, then maximize again. Otherwise center and clamp size
    const X11::WindowProperty class_property = X11::get_window_property(window, X11::atoms.window_class);
    const std::string window_name = class_property.property_value == nullptr ? "NONE" : std::string((const char*)class_property.property_value);
    const bool b_begin_maximized = EshyWMConfig::window_close_data.contains(window_name) && EshyWMConfig::window_close_data[window_name] == "maximized";
    maximize_window(b_begin_maximized);
}

void EshyWMWindow::frame_window()
{
    const auto offset = Pos{ 0, EshyWMConfig::titlebar ? (int)EshyWMConfig::titlebar_height : 0 };
    const auto border_width = EshyWM::window_manager->b_show_window_borders ? EshyWMConfig::window_frame_border_width : 0;
    frame = X11::create_window(frame_geometry, SubstructureRedirectMask | EnterWindowMask, border_width);
    X11::reparent_window(window, frame, offset);

    //In EshyWM, we set frame class to match the window to support compositors
    if (const X11::WindowProperty class_property = X11::get_window_property(window, X11::atoms.window_class))
        X11::change_window_property(frame, X11::atoms.window_class, XA_STRING, 8, (unsigned char*)class_property.property_value);

    X11::map_window(frame);

    const auto titlebar_geometry = Rect{ 0, 0, frame_geometry.width, EshyWMConfig::titlebar_height };
    titlebar = X11::create_window(titlebar_geometry, VisibilityChangeMask, 0);
    X11::reparent_window(titlebar, frame, { 0 });

    const auto initial_size = Rect{ 0, 0, EshyWMConfig::titlebar_button_size, EshyWMConfig::titlebar_button_size };
    const auto close_button_color = button_color_data{ EshyWMConfig::titlebar_button_normal_color, EshyWMConfig::close_button_color, EshyWMConfig::titlebar_button_pressed_color };
    close_button = new ImageButton(titlebar, initial_size, close_button_color, EshyWMConfig::close_button_image_path.c_str());
    close_button->click_callback = std::bind(std::mem_fn(&EshyWMWindow::close_window), this);

    set_show_titlebar(EshyWMConfig::titlebar);

    XFlush(X11::get_display());
    set_window_state(WS_NORMAL);
}

void EshyWMWindow::unframe_window()
{
    X11::reparent_window(window, X11::get_root_window(), { 0 });
    X11::destroy_window(frame);
    X11::destroy_window(titlebar);
}


void EshyWMWindow::set_window_state(EWindowState new_window_state)
{
    const std::unordered_map<EWindowState, const char*> states = {
        {WS_NONE, "none"},
        {WS_NORMAL, "normal"},
        {WS_MINIMIZED, "minimized"},
        {WS_MAXIMIZED, "maximized"},
        {WS_FULLSCREEN, "fullscreen"},
        {WS_ANCHORED_LEFT, "anchored_left"},
        {WS_ANCHORED_UP, "anchored_up"},
        {WS_ANCHORED_RIGHT, "anchored_right"},
        {WS_ANCHORED_DOWN, "anchored_down"}
    };

    XStoreName(X11::get_display(), frame, states.at(new_window_state));
    previous_state = window_state;
    window_state = new_window_state;
}

void EshyWMWindow::minimize_window(bool b_minimize)
{
    if (b_minimize && window_state != WS_MINIMIZED)
    {
        X11::unmap_window(frame);
        fullscreen_window(false);
        set_window_state(WS_MINIMIZED);
    }
    else if (!b_minimize && window_state == WS_MINIMIZED)
    {
        //Do not raise if parent workspace is not active
        if (!parent_workspace->b_is_active)
            return;

        set_window_state(previous_state);
        X11::map_window(frame);
        update_titlebar();

        //Restore the window to its previous state
        if (window_state == WS_MAXIMIZED)
            maximize_window(true);
        else if (window_state == WS_FULLSCREEN)
            fullscreen_window(true);
        else if (window_state >= WS_ANCHORED_LEFT)
            anchor_window(window_state);
    }
}

void EshyWMWindow::maximize_window(bool b_maximize)
{
    if (b_maximize && window_state != WS_MAXIMIZED)
    {
        fullscreen_window(false);

        if (window_state == WS_NORMAL)
            pre_state_change_geometry = frame_geometry;

        move_window_absolute(parent_workspace->geometry.x, parent_workspace->geometry.y, true);
        const int width = parent_workspace->geometry.width - (EshyWMConfig::window_frame_border_width * 2);
        const int height = parent_workspace->geometry.height - (EshyWMConfig::window_frame_border_width * 2) - (EshyWMConfig::titlebar * EshyWMConfig::titlebar_height);
        resize_window_absolute(width, height, true);
        set_window_state(WS_MAXIMIZED);
    }
    else if (!b_maximize && window_state == WS_MAXIMIZED)
    {
        move_window_absolute(pre_state_change_geometry.x, pre_state_change_geometry.y, true);
        resize_window_absolute(pre_state_change_geometry.width, pre_state_change_geometry.height, true);
        set_window_state(previous_state);
    }
}

void EshyWMWindow::fullscreen_window(bool b_fullscreen)
{
    if (b_fullscreen && window_state != WS_FULLSCREEN)
    {
        if (window_state == WS_NORMAL)
            pre_state_change_geometry = frame_geometry;

        set_show_titlebar(false);
        move_window_absolute(parent_workspace->parent_output->geometry.x, parent_workspace->parent_output->geometry.y, true);
        resize_window_absolute(parent_workspace->parent_output->geometry.width, parent_workspace->parent_output->geometry.height, true);
        set_window_state(WS_FULLSCREEN);
    }
    else if (!b_fullscreen && window_state == WS_FULLSCREEN)
    {
        set_show_titlebar(true);
        move_window_absolute(pre_state_change_geometry.x, pre_state_change_geometry.y, true);
        resize_window_absolute(pre_state_change_geometry.width, pre_state_change_geometry.height, true);
        set_window_state(previous_state);

        //Restore the window to its previous state
        if (window_state == WS_MAXIMIZED)
        {
            window_state = WS_NONE;
            maximize_window(true);
        }
        else if (window_state == WS_FULLSCREEN)
        {
            window_state = WS_NONE;
            fullscreen_window(true);
        }
        else if (window_state >= WS_ANCHORED_LEFT)
            anchor_window(window_state);
    }
}

void EshyWMWindow::close_window()
{
    X11::unmap_window(window);
    unframe_window();

    if (!X11::close_window(window))
    {
        X11::kill_window(window);
    }

    if (const X11::WindowProperty property = X11::get_window_property(window, X11::atoms.window_class))
    {
        EshyWMConfig::add_window_close_state((const char*)property.property_value, get_window_state() == EWindowState::WS_MAXIMIZED ? "maximized" : "normal");
    }
}


void EshyWMWindow::anchor_window(EWindowState anchor, std::shared_ptr<Output> output_override)
{
    if (window_state == anchor)
    {
        attempt_shift_monitor_anchor(anchor);
        return;
    }

    if (window_state == WS_NORMAL)
        pre_state_change_geometry = frame_geometry;

    switch (anchor)
    {
    case WS_ANCHORED_LEFT:
    {
        if (window_state == WS_ANCHORED_RIGHT && !output_override) goto set_normal;
        move_window_absolute(parent_workspace->geometry.x, parent_workspace->geometry.y, true);
        resize_window_absolute(half_of(parent_workspace->geometry.width), parent_workspace->geometry.height, true);
        break;
    }
    case WS_ANCHORED_UP:
    {
        if (window_state == WS_ANCHORED_DOWN && !output_override) goto set_normal;
        move_window_absolute(parent_workspace->geometry.x, parent_workspace->geometry.y, true);
        resize_window_absolute(parent_workspace->geometry.width, half_of(parent_workspace->geometry.height), true);
        break;
    }
    case WS_ANCHORED_RIGHT:
    {
        if (window_state == WS_ANCHORED_LEFT && !output_override) goto set_normal;
        move_window_absolute(parent_workspace->geometry.x + half_of(parent_workspace->geometry.width), parent_workspace->geometry.y, true);
        resize_window_absolute(half_of(parent_workspace->geometry.width), parent_workspace->geometry.height, true);
        break;
    }
    case WS_ANCHORED_DOWN:
    {
        if (window_state == WS_ANCHORED_UP && !output_override) goto set_normal;
        move_window_absolute(parent_workspace->geometry.x, parent_workspace->geometry.y + half_of(parent_workspace->geometry.height), true);
        resize_window_absolute(parent_workspace->geometry.width, half_of(parent_workspace->geometry.height), true);
        break;
    }
    case WS_NORMAL:
    {
    set_normal:
        set_window_state(WS_NORMAL);
        move_window_absolute(pre_state_change_geometry.x, pre_state_change_geometry.y, true);
        resize_window_absolute(pre_state_change_geometry.width, pre_state_change_geometry.height, true);
        return;
    }
    };

    set_window_state(anchor);
}

void EshyWMWindow::attempt_shift_monitor_anchor(EWindowState direction)
{
    int test_x = 0;
    int test_y = 0;
    EWindowState anchor = WS_NONE;
    Rect new_pre_state_change_geometry = pre_state_change_geometry;

    switch (direction)
    {
    case WS_ANCHORED_LEFT:
    {
        test_x = parent_workspace->parent_output->geometry.x - 10;
        test_y = parent_workspace->parent_output->geometry.y;
        anchor = WS_ANCHORED_RIGHT;
        new_pre_state_change_geometry.x -= parent_workspace->parent_output->geometry.width;
        break;
    }
    case WS_ANCHORED_UP:
    {
        test_x = parent_workspace->parent_output->geometry.x;
        test_y = parent_workspace->parent_output->geometry.y - 10;
        anchor = WS_ANCHORED_DOWN;
        new_pre_state_change_geometry.y -= parent_workspace->parent_output->geometry.height;
        break;
    }
    case WS_ANCHORED_RIGHT:
    {
        test_x = parent_workspace->parent_output->geometry.x + parent_workspace->parent_output->geometry.width + 10;
        test_y = parent_workspace->parent_output->geometry.y;
        anchor = WS_ANCHORED_LEFT;
        new_pre_state_change_geometry.x += parent_workspace->parent_output->geometry.width;
        break;
    }
    case WS_ANCHORED_DOWN:
    {
        test_x = parent_workspace->parent_output->geometry.x;
        test_y = parent_workspace->parent_output->geometry.y + parent_workspace->parent_output->geometry.height + 10;
        anchor = WS_ANCHORED_UP;
        new_pre_state_change_geometry.y += parent_workspace->parent_output->geometry.height;
        break;
    }
    };

    if (auto new_output = output_at_position(test_x, test_y))
    {
        pre_state_change_geometry = new_pre_state_change_geometry;
        pre_state_change_geometry.width *= parent_workspace->parent_output->geometry.width / new_output->geometry.width;
        pre_state_change_geometry.height *= parent_workspace->parent_output->geometry.height / new_output->geometry.height;
        anchor_window(anchor, new_output);
    }
}

void EshyWMWindow::attempt_shift_monitor(EWindowState direction)
{
    int test_x = 0;
    int test_y = 0;
    Rect new_pre_state_change_geometry = pre_state_change_geometry;

    switch (direction)
    {
    case WS_ANCHORED_LEFT:
    {
        test_x = parent_workspace->parent_output->geometry.x - 10;
        test_y = parent_workspace->parent_output->geometry.y;
        new_pre_state_change_geometry.x -= parent_workspace->parent_output->geometry.width;
        break;
    }
    case WS_ANCHORED_UP:
    {
        test_x = parent_workspace->parent_output->geometry.x;
        test_y = parent_workspace->parent_output->geometry.y - 10;
        new_pre_state_change_geometry.y -= parent_workspace->parent_output->geometry.height;
        break;
    }
    case WS_ANCHORED_RIGHT:
    {
        test_x = parent_workspace->parent_output->geometry.x + parent_workspace->parent_output->geometry.width + 10;
        test_y = parent_workspace->parent_output->geometry.y;
        new_pre_state_change_geometry.x += parent_workspace->parent_output->geometry.width;
        break;
    }
    case WS_ANCHORED_DOWN:
    {
        test_x = parent_workspace->parent_output->geometry.x;
        test_y = parent_workspace->parent_output->geometry.y + parent_workspace->parent_output->geometry.height + 10;
        new_pre_state_change_geometry.y += parent_workspace->parent_output->geometry.height;
        break;
    }
    };

    if (auto output = output_at_position(test_x, test_y))
    {
        pre_state_change_geometry = new_pre_state_change_geometry;
        pre_state_change_geometry.width *= parent_workspace->parent_output->geometry.width / output->geometry.width;
        pre_state_change_geometry.height *= parent_workspace->parent_output->geometry.height / output->geometry.height;
        move_window_absolute(pre_state_change_geometry.x, pre_state_change_geometry.y, false);
    }
}


void EshyWMWindow::move_window_absolute(int new_position_x, int new_position_y, bool b_skip_state_checks)
{
    if (!b_skip_state_checks)
    {
        if (window_state == WS_MAXIMIZED)
            maximize_window(false);
        else if (window_state == WS_FULLSCREEN)
            fullscreen_window(false);
        else if (window_state >= WS_ANCHORED_LEFT)
            anchor_window(WS_NORMAL);
    }

    frame_geometry.x = new_position_x;
    frame_geometry.y = new_position_y;
    X11::move_window(frame, frame_geometry);

    //Update the workspace it is in
    if (auto output = output_most_occupied(frame_geometry))
    {
        parent_workspace = output->active_workspace;
    }
}

void EshyWMWindow::resize_window_absolute(uint new_size_x, uint new_size_y, bool b_skip_state_checks)
{
    if (!b_skip_state_checks)
    {
        if (window_state == WS_MAXIMIZED)
            maximize_window(false);
        else if (window_state == WS_FULLSCREEN)
            fullscreen_window(false);
    }

    frame_geometry.width = new_size_x;
    frame_geometry.height = new_size_y + (b_show_titlebar * EshyWMConfig::titlebar_height);

    X11::resize_window(frame, frame_geometry);
    X11::resize_window(window, Size{ new_size_x, new_size_y });

    if (EshyWMConfig::titlebar)
    {
        X11::resize_window(titlebar, Size{ new_size_x, EshyWMConfig::titlebar_height });
        update_titlebar();
    }

    //Update the workspace it is in
    if (auto output = output_most_occupied(frame_geometry))
        parent_workspace = output->active_workspace;
}


void EshyWMWindow::set_show_border(bool b_show_border)
{
    if (b_show_border)
    {
        X11::set_border_color(frame, EshyWMConfig::window_frame_border_color);
        X11::set_border_width(frame, EshyWMConfig::window_frame_border_width);
    }
    else
    {
        X11::set_border_width(frame, 0);
    }
}

void EshyWMWindow::set_show_titlebar(bool b_new_show_titlebar)
{
    if (b_show_titlebar == b_new_show_titlebar)
        return;

    b_show_titlebar = b_new_show_titlebar;

    if (b_show_titlebar)
    {
        X11::map_window(titlebar);
        X11::move_window(window, Pos{ 0, (int)EshyWMConfig::titlebar_height });
        resize_window_absolute(frame_geometry.width, frame_geometry.height, true);
    }
    else
    {
        X11::unmap_window(titlebar);
        X11::move_window(window, Pos{ 0 });
        resize_window_absolute(frame_geometry.width, frame_geometry.height, true);
    }
}

void EshyWMWindow::update_titlebar()
{
    if (!EshyWMConfig::titlebar || !b_show_titlebar)
        return;

    XClearWindow(X11::get_display(), titlebar);

    if (window_font)
    {
        XTextProperty name;
        XGetWMName(X11::get_display(), window, &name);
        const auto display_name = (char*)name.value;

        Imlib_Image buffer = imlib_create_image(frame_geometry.width, EshyWMConfig::titlebar_height);

        imlib_context_set_image(buffer);
        imlib_context_set_font(window_font);

        const ulong backgound_color = EshyWMConfig::window_background_color;
        const int r = backgound_color >> 16 & 0xFF;
        const int g = backgound_color >> 8 & 0xFF;
        const int b = backgound_color & 0xFF;
        imlib_context_set_color(r, g, b, 255);
        imlib_image_fill_rectangle(0, 0, frame_geometry.width, EshyWMConfig::titlebar_height);

        imlib_context_set_color(255, 255, 255, 255);
        imlib_text_draw(EshyWMConfig::titlebar_height + 8, 0, display_name);

        imlib_context_set_drawable(titlebar);
        imlib_context_set_blend(0);
        imlib_render_image_on_drawable(0, 0);

        XFree(name.value);
    }

    if (window_icon)
        window_icon->draw(titlebar, 8, 4, EshyWMConfig::titlebar_height - 8, EshyWMConfig::titlebar_height - 8);

    const int button_y_offset = (EshyWMConfig::titlebar_height - EshyWMConfig::titlebar_button_size) / 2;
    close_button->set_position((frame_geometry.width - EshyWMConfig::titlebar_button_size) - button_y_offset, button_y_offset);
    close_button->draw();
}
