#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <cairo/cairo-xlib.h>

#include "window_manager.h"
#include "window.h"
#include "config.h"
#include "eshywm.h"
#include "button.h"
#include "switcher.h"
#include "util.h"
#include "X11.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <span>

constexpr int half_of(const int x)
{
    return x / 2;
}

static bool retrieve_window_icon(Window window, Imlib_Image* icon)
{
    Atom type_return;
    int format_return;
    unsigned long nitems_return;
    unsigned long bytes_after_return;
    unsigned char* data_return = nullptr;

    int status = XGetWindowProperty(X11::get_display(), window, X11::atoms.window_icon, 0, 1, false, X11::atoms.cardinal, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
    if (status == Success && data_return)
    {
        const int width = *(unsigned int*)data_return;
        XFree(data_return);

        XGetWindowProperty(X11::get_display(), window, X11::atoms.window_icon, 1, 1, false, X11::atoms.cardinal, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
        const int height = *(unsigned int*)data_return;
        XFree(data_return);

        XGetWindowProperty(X11::get_display(), window, X11::atoms.window_icon, 2, width * height, false, X11::atoms.cardinal, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
        uint32_t* img_data = new uint32_t[width * height];
        const ulong* ul = (ulong*)data_return;

        for(int i = 0; i < nitems_return; i++)
            img_data[i] = (uint32_t)ul[i];

        XFree(data_return);

        *icon = imlib_create_image_using_data(width, height, img_data);
        if(*icon)
            return true;
    }
    
    status = XGetWindowProperty(X11::get_display(), window, X11::atoms.window_icon_name, 0, 1024, False, AnyPropertyType, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
    if(status == Success && type_return != None && format_return == 8 && data_return)
    {
        *icon = imlib_load_image(std::string("/usr/share/icons/hicolor/256x256/apps/" + std::string((const char*)data_return) + ".png").c_str());
        XFree(data_return);
        if(*icon)
            return true;
    }

    status = XGetWindowProperty(X11::get_display(), window, X11::atoms.window_class, 0, 1024, False, AnyPropertyType, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
    if(status == Success && type_return != None && format_return == 8 && data_return)
    {
        std::ifstream desktop_file("/usr/share/applications/" + std::string((const char*)data_return) + ".desktop");
        std::cout << (const char*)data_return << std::endl;

        if(!desktop_file.is_open())
            return false;

        std::string line;
        std::string icon_name;

        while(getline(desktop_file, line))
        {
            if(line.find("Icon") != std::string::npos)
            {
                size_t Del = line.find("=");
                icon_name = line.substr(Del + 1);
                break;
            }
        }

        desktop_file.close();
        XFree(data_return);

        *icon = imlib_load_image(std::string("/usr/share/icons/hicolor/256x256/apps/" + icon_name + ".png").c_str());
        if(*icon)
            return true;
    }

    return false;
}

EshyWMWindow::EshyWMWindow(Window _window, Workspace* _workspace, const Rect& geometry)
    : window(_window)
    , parent_workspace(_workspace)
    , b_show_titlebar(true)
    , window_icon(nullptr)
    , frame_geometry(geometry)
    , pre_state_change_geometry(geometry)
    , window_state(WS_NONE)
{
    if(!retrieve_window_icon(window, &window_icon))
        window_icon = imlib_load_image(EshyWMConfig::default_application_image_path.c_str());
}

EshyWMWindow::~EshyWMWindow()
{
    imlib_context_set_image(window_icon);
    imlib_free_image();

    delete close_button;
}


void EshyWMWindow::frame_window()
{
    const Pos offset = {0, (int)EshyWMConfig::titlebar_height};
    const Rect titlebar_geometry = {0, 0, (uint)frame_geometry.width, (uint)EshyWMConfig::titlebar_height};

    //Frame the window
    frame = X11::create_window(frame_geometry, SubstructureRedirectMask | EnterWindowMask, EshyWMConfig::window_frame_border_width);
    X11::reparent_window(window, frame, offset);

    //In EshyWM, we set frame class to match the window to support compositors
    if(const X11::WindowProperty class_property = X11::get_window_property(window, X11::atoms.window_class))
    {
        X11::change_window_property(frame, X11::atoms.window_class, XA_STRING, 8, (unsigned char*)class_property.property_value);
    }

    X11::map_window(frame);

    //Add a titlebar to the window
    titlebar = X11::create_window(titlebar_geometry, VisibilityChangeMask, 0);
    X11::reparent_window(titlebar, frame, {0});
    X11::map_window(titlebar);

    cairo_titlebar_surface = cairo_xlib_surface_create(X11::get_display(), titlebar, DefaultVisual(X11::get_display(), 0), frame_geometry.width, EshyWMConfig::titlebar_height);
    cairo_context = cairo_create(cairo_titlebar_surface);

    const Rect initial_size = {0, 0, EshyWMConfig::titlebar_button_size, EshyWMConfig::titlebar_button_size};
    const button_color_data close_button_color = {EshyWMConfig::titlebar_button_normal_color, EshyWMConfig::close_button_color, EshyWMConfig::titlebar_button_pressed_color};
    close_button = new ImageButton(titlebar, initial_size, close_button_color, EshyWMConfig::close_button_image_path.c_str());
    close_button->click_callback = std::bind(std::mem_fn(&EshyWMWindow::close_window), this);

    XFlush(X11::get_display());
    set_window_state(WS_NORMAL);
    update_titlebar();
}

void EshyWMWindow::unframe_window()
{
    X11::reparent_window(window, X11::get_root_window(), {0});
    X11::destroy_window(frame);
    X11::destroy_window(titlebar);

    if(cairo_titlebar_surface && cairo_context)
    {
        cairo_surface_destroy(cairo_titlebar_surface);
        cairo_destroy(cairo_context);
        cairo_titlebar_surface = nullptr;
        cairo_context = nullptr;
    }
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
    if(b_minimize && window_state != WS_MINIMIZED)
    {
        X11::unmap_window(frame);
        fullscreen_window(false);
        set_window_state(WS_MINIMIZED);
    }
    else if (!b_minimize && window_state == WS_MINIMIZED)
    {
        //Do not raise if parent workspace is not active
        if(!parent_workspace->b_is_active)
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
    if(b_maximize && window_state != WS_MAXIMIZED)
    {
        fullscreen_window(false);

        if (window_state == WS_NORMAL)
            pre_state_change_geometry = frame_geometry;

        move_window_absolute(parent_workspace->geometry.x, parent_workspace->geometry.y, true);
        resize_window_absolute(parent_workspace->geometry.width - (EshyWMConfig::window_frame_border_width * 2), parent_workspace->geometry.height - (EshyWMConfig::window_frame_border_width * 2), true);
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
    if(b_fullscreen && window_state != WS_FULLSCREEN)
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

    const bool b_close_successful = X11::close_window(window);
    if(!X11::close_window(window))
    {
        X11::kill_window(window);
    }

    if(const X11::WindowProperty property = X11::get_window_property(window, X11::atoms.window_class))
    {
        EshyWMConfig::add_window_close_state((const char*)property.property_value, get_window_state() == EWindowState::WS_MAXIMIZED ? "maximized" : "normal");
    }
}


void EshyWMWindow::anchor_window(EWindowState anchor, Output* output_override)
{
    if(window_state == anchor)
    {
        attempt_shift_monitor_anchor(anchor);
        return;
    }

    if(window_state == WS_NORMAL)
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

    switch(direction)
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

    if(Output* new_output = output_at_position(test_x, test_y))
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

    switch(direction)
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

    if(Output* output = output_at_position(test_x, test_y))
    {
        pre_state_change_geometry = new_pre_state_change_geometry;
        pre_state_change_geometry.width *= parent_workspace->parent_output->geometry.width / output->geometry.width;
        pre_state_change_geometry.height *= parent_workspace->parent_output->geometry.height / output->geometry.height;
        move_window_absolute(pre_state_change_geometry.x, pre_state_change_geometry.y, false);
    }
}


void EshyWMWindow::move_window_absolute(int new_position_x, int new_position_y, bool b_skip_state_checks)
{
    if(!b_skip_state_checks)
    {
        if(window_state == WS_MAXIMIZED)
            maximize_window(false);
        else if(window_state == WS_FULLSCREEN)
            fullscreen_window(false);
        else if(window_state >= WS_ANCHORED_LEFT)
            anchor_window(WS_NORMAL);
    }

    frame_geometry.x = new_position_x;
    frame_geometry.y = new_position_y;
    X11::move_window(frame, frame_geometry);

    //Update the workspace it is in
    if(Output* output = output_most_occupied(frame_geometry))
    {
        parent_workspace = output->active_workspace;
    }
}

void EshyWMWindow::resize_window_absolute(uint new_size_x, uint new_size_y, bool b_skip_state_checks)
{
    if(!b_skip_state_checks)
    {
        if(window_state == WS_MAXIMIZED)
            maximize_window(false);
        else if(window_state == WS_FULLSCREEN)
            fullscreen_window(false);
    }

    frame_geometry.width = new_size_x;
    frame_geometry.height = new_size_y;

    X11::resize_window(frame, frame_geometry);
    X11::resize_window(window, Size{new_size_x, new_size_y - (b_show_titlebar ? EshyWMConfig::titlebar_height : 0)});
    X11::resize_window(titlebar, Size{new_size_x, EshyWMConfig::titlebar_height});
    update_titlebar();

    //Update the workspace it is in
    if(Output* output = output_most_occupied(frame_geometry))
        parent_workspace = output->active_workspace;
}


void EshyWMWindow::set_show_titlebar(bool b_new_show_titlebar)
{
    if(b_show_titlebar == b_new_show_titlebar)
        return;
    
    b_show_titlebar = b_new_show_titlebar;

    if(b_show_titlebar)
    {
        X11::map_window(titlebar);
        X11::move_window(window, Pos{0, (int)EshyWMConfig::titlebar_height});
        resize_window_absolute(frame_geometry.width, frame_geometry.height, true);
    }
    else
    {
        X11::unmap_window(titlebar);
        X11::move_window(window, Pos{0});
        resize_window_absolute(frame_geometry.width, frame_geometry.height, true);
    }
}

void EshyWMWindow::update_titlebar()
{
    if(!b_show_titlebar || !cairo_context)
        return;
    
    XClearWindow(X11::get_display(), titlebar);

    XTextProperty name;
    XGetWMName(X11::get_display(), window, &name);

    cairo_select_font_face(cairo_context, "Lato", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cairo_context, 16);
    cairo_set_source_rgb(cairo_context, 1.0f, 1.0f, 1.0f);
    cairo_move_to(cairo_context, 10.0f, 16.0f);
    cairo_show_text(cairo_context, (char*)name.value);

    const int button_y_offset = (EshyWMConfig::titlebar_height - EshyWMConfig::titlebar_button_size) / 2;
    close_button->set_position((frame_geometry.width - EshyWMConfig::titlebar_button_size) - button_y_offset, button_y_offset);
    close_button->draw();
}
