#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <cairo/cairo-xlib.h>

#include "window_manager.h"
#include "window.h"
#include "config.h"
#include "eshywm.h"
#include "button.h"
#include "taskbar.h"
#include "switcher.h"
#include "util.h"

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
    static const Atom ATOM_NET_WM_ICON = XInternAtom(DISPLAY, "_NET_WM_ICON", false);
    static const Atom ATOM_CARDINAL = XInternAtom(DISPLAY, "CARDINAL", false);
    static const Atom ATOM_CLASS = XInternAtom(DISPLAY, "WM_CLASS", False);
    static const Atom ATOM_ICON_NAME = XInternAtom(DISPLAY, "WM_ICON_NAME", False);

    Atom type_return;
    int format_return;
    unsigned long nitems_return;
    unsigned long bytes_after_return;
    unsigned char* data_return = nullptr;

    int status = XGetWindowProperty(DISPLAY, window, ATOM_NET_WM_ICON, 0, 1, false, ATOM_CARDINAL, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
    if (status == Success && data_return)
    {
        const int width = *(unsigned int*)data_return;
        XFree(data_return);

        XGetWindowProperty(DISPLAY, window, ATOM_NET_WM_ICON, 1, 1, false, ATOM_CARDINAL, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
        const int height = *(unsigned int*)data_return;
        XFree(data_return);

        XGetWindowProperty(DISPLAY, window, ATOM_NET_WM_ICON, 2, width * height, false, ATOM_CARDINAL, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
        uint32_t* img_data = new uint32_t[width * height];
        const ulong* ul = (ulong*)data_return;

        for(int i = 0; i < nitems_return; i++)
            img_data[i] = (uint32_t)ul[i];

        XFree(data_return);

        *icon = imlib_create_image_using_data(width, height, img_data);
        if(*icon)
            return true;
    }
    
    status = XGetWindowProperty(DISPLAY, window, ATOM_ICON_NAME, 0, 1024, False, AnyPropertyType, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
    if(status == Success && type_return != None && format_return == 8 && data_return)
    {
        *icon = imlib_load_image(std::string("/usr/share/icons/hicolor/256x256/apps/" + std::string((const char*)data_return) + ".png").c_str());
        XFree(data_return);
        if(*icon)
            return true;
    }

    status = XGetWindowProperty(DISPLAY, window, ATOM_CLASS, 0, 1024, False, AnyPropertyType, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
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

EshyWMWindow::EshyWMWindow(Window _window, Workspace* _workspace, bool _b_force_no_titlebar)
    : window(_window)
    , parent_workspace(_workspace)
    , b_force_no_titlebar(_b_force_no_titlebar)
    , b_show_titlebar(!_b_force_no_titlebar)
    , window_icon(nullptr)
    , frame_geometry({0, 0, 100, 100})
    , pre_state_change_geometry({0, 0, 100, 100})
{
    if(!retrieve_window_icon(window, &window_icon))
        window_icon = imlib_load_image(EshyWMConfig::default_application_image_path.c_str());
}

EshyWMWindow::~EshyWMWindow()
{
    imlib_context_set_image(window_icon);
    imlib_free_image();

    cairo_surface_destroy(cairo_titlebar_surface);
    cairo_destroy(cairo_context);
}


void EshyWMWindow::frame_window(XWindowAttributes attr)
{
    frame_geometry = {attr.x, attr.y, (uint)attr.width, (uint)(attr.height + EshyWMConfig::titlebar_height)};
    pre_state_change_geometry = frame_geometry;

    frame = XCreateSimpleWindow(DISPLAY, ROOT, frame_geometry.x, frame_geometry.y, frame_geometry.width, frame_geometry.height, EshyWMConfig::window_frame_border_width, EshyWMConfig::window_frame_border_color, EshyWMConfig::window_background_color);
    //Resize because in case we use the * 0.9 version of height, then the bottom is cut off
    XResizeWindow(DISPLAY, window, attr.width, attr.height);
    XReparentWindow(DISPLAY, window, frame, 0, (!b_force_no_titlebar ? EshyWMConfig::titlebar_height : 0));
    XSelectInput(DISPLAY, frame, EnterWindowMask);
    XMapWindow(DISPLAY, frame);

    XSelectInput(DISPLAY, window, PointerMotionMask | StructureNotifyMask | PropertyChangeMask);

    set_window_state(WS_NORMAL);

    //Set frame class to match the window
    const XPropertyReturn class_property = get_xwindow_property(DISPLAY, window, atoms.window_class);
    if(class_property.status == Success && class_property.type != None && class_property.format == 8)
        XChangeProperty(DISPLAY, frame, atoms.window_class, XA_STRING, 8, PropModeReplace, (unsigned char*)class_property.property_value, strlen((const char*)class_property.property_value));

    XFree(class_property.property_value);
    XFlush(DISPLAY);

    if(!b_force_no_titlebar)
    {
        titlebar = XCreateSimpleWindow(DISPLAY, frame, 0, 0, attr.width, EshyWMConfig::titlebar_height, 0, EshyWMConfig::window_frame_border_color, EshyWMConfig::window_background_color);
        XSelectInput(DISPLAY, titlebar, VisibilityChangeMask);
        XMapWindow(DISPLAY, titlebar);

        cairo_titlebar_surface = cairo_xlib_surface_create(DISPLAY, titlebar, DefaultVisual(DISPLAY, 0), attr.width, EshyWMConfig::titlebar_height);
        cairo_context = cairo_create(cairo_titlebar_surface);
        
        const Rect initial_size = {0, 0, EshyWMConfig::titlebar_button_size, EshyWMConfig::titlebar_button_size};
        const button_color_data close_button_color = {EshyWMConfig::titlebar_button_normal_color, EshyWMConfig::close_button_color, EshyWMConfig::titlebar_button_pressed_color};
        close_button = std::make_shared<ImageButton>(titlebar, initial_size, close_button_color, EshyWMConfig::close_button_image_path.c_str());
        close_button->click_callback = std::bind(std::mem_fn(&EshyWMWindow::close_window), this);
    }

    update_titlebar();
}

void EshyWMWindow::unframe_window()
{
    if(frame)
    {
        XUnmapWindow(DISPLAY, frame);
        XReparentWindow(DISPLAY, window, ROOT, 0, 0);
        XRemoveFromSaveSet(DISPLAY, window);
        XDestroyWindow(DISPLAY, frame);
    }

    if(titlebar)
    {        
        XUnmapWindow(DISPLAY, titlebar);
        XDestroyWindow(DISPLAY, titlebar);
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
    
    XStoreName(DISPLAY, frame, states.at(new_window_state));
    previous_state = window_state;
    window_state = new_window_state;
}

void EshyWMWindow::minimize_window(bool b_minimize)
{
    if(b_minimize && window_state != WS_MINIMIZED)
    {
        XUnmapWindow(DISPLAY, get_frame());
        fullscreen_window(false);
        set_window_state(WS_MINIMIZED);
    }
    else if (!b_minimize && window_state == WS_MINIMIZED)
    {
        //Do not raise if parent workspace is not active
        if(!parent_workspace->b_is_active)
            return;

        set_window_state(previous_state);
        XMapWindow(DISPLAY, get_frame());
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
    XUnmapWindow(DISPLAY, window);
    unframe_window();

    Atom* supported_protocols;
    int num_supported_protocols;

    //This is not a typical status return. This one is zero when it fails.
    const int status = XGetWMProtocols(DISPLAY, window, &supported_protocols, &num_supported_protocols);

    const bool b_protocol_exists = std::ranges::contains(std::span(supported_protocols, num_supported_protocols), atoms.wm_delete_window);
    if (status && b_protocol_exists)
    {
        XEvent message;
        memset(&message, 0, sizeof(message));
        message.xclient.type = ClientMessage;
        message.xclient.message_type = atoms.wm_protocols;
        message.xclient.window = window;
        message.xclient.format = 32;
        message.xclient.data.l[0] = atoms.wm_delete_window;
        XSendEvent(DISPLAY, window, false, 0 , &message);
    }
    else
    {
        XKillClient(DISPLAY, window);
    }

    const XPropertyReturn property = get_xwindow_property(DISPLAY, window, atoms.window_class);
    if(property.status == Success)
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
    XMoveWindow(DISPLAY, frame, new_position_x, new_position_y);

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

    XResizeWindow(DISPLAY, frame, new_size_x, new_size_y);
    XResizeWindow(DISPLAY, window, new_size_x, new_size_y - (b_show_titlebar ? EshyWMConfig::titlebar_height : 0));
    XResizeWindow(DISPLAY, titlebar, new_size_x, EshyWMConfig::titlebar_height);
    update_titlebar();

    //Update the workspace it is in
    if(Output* output = output_most_occupied(frame_geometry))
        parent_workspace = output->active_workspace;
}


void EshyWMWindow::set_show_titlebar(bool b_new_show_titlebar)
{
    if(b_force_no_titlebar || b_show_titlebar == b_new_show_titlebar)
        return;
    
    b_show_titlebar = b_new_show_titlebar;

    if(b_show_titlebar)
    {
        XMapWindow(DISPLAY, titlebar);
        XMoveWindow(DISPLAY, window, 0, EshyWMConfig::titlebar_height);
        resize_window_absolute(frame_geometry.width, frame_geometry.height, true);
    }
    else
    {
        XUnmapWindow(DISPLAY, titlebar);
        XMoveWindow(DISPLAY, window, 0, 0);
        resize_window_absolute(frame_geometry.width, frame_geometry.height, true);
    }
}

void EshyWMWindow::update_titlebar()
{
    if(!b_show_titlebar || !cairo_context)
        return;
    
    XClearWindow(DISPLAY, titlebar);

    XTextProperty name;
    XGetWMName(DISPLAY, window, &name);

    cairo_select_font_face(cairo_context, "Lato", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cairo_context, 16);
    cairo_set_source_rgb(cairo_context, 1.0f, 1.0f, 1.0f);
    cairo_move_to(cairo_context, 10.0f, 16.0f);
    cairo_show_text(cairo_context, (char*)name.value);

    const int button_y_offset = (EshyWMConfig::titlebar_height - EshyWMConfig::titlebar_button_size) / 2;
    close_button->set_position((frame_geometry.width - EshyWMConfig::titlebar_button_size) - button_y_offset, button_y_offset);
    close_button->draw();
}
