#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <cairo/cairo-xlib.h>

#include "window_manager.h"
#include "window.h"
#include "config.h"
#include "eshywm.h"
#include "button.h"
#include "taskbar.h"

#include <algorithm>
#include <cstring>

const int majority_monitor(rect window_geometry)
{
    int monitor = WindowManager::monitor_info.size() - 1;
    for(std::vector<s_monitor_info>::reverse_iterator it = WindowManager::monitor_info.rbegin(); it != WindowManager::monitor_info.rend(); it++)
    {
        if((window_geometry.x + (window_geometry.width / 2)) > (*it).x)
        {
            break;
        }

        monitor--;
    }
    return monitor;
}

namespace WindowManager
{
void _minimize_window(std::shared_ptr<EshyWMWindow> window, void* null)
{
    if(!window->is_minimized())
    {
        XUnmapWindow(DISPLAY, window->get_frame());
    }
    else
    {
        XMapWindow(DISPLAY, window->get_frame());
        window->update_titlebar();
    }

    window->set_minimized(!window->is_minimized());
}

void _maximize_window(std::shared_ptr<EshyWMWindow> window, void* b_from_move_or_resize)
{
    if(!window->is_maximized())
    {
        if(window->is_fullscreen())
            _fullscreen_window(window, b_from_move_or_resize);
        
        window->pre_state_change_geoemtry = window->get_frame_geometry();
        const int i = majority_monitor(window->get_frame_geometry());
        window->move_window_absolute(DISPLAY_X(i), DISPLAY_Y(i), true);
        window->resize_window_absolute(DISPLAY_WIDTH(i) - (EshyWMConfig::window_frame_border_width * 2), DISPLAY_HEIGHT(i) - (EshyWMConfig::window_frame_border_width * 2), true);

        XStoreName(DISPLAY, window->get_frame(), "maximized");
    }
    else
    {
        if(!(bool*)(b_from_move_or_resize))
        {
            window->move_window_absolute(window->pre_state_change_geoemtry.x, window->pre_state_change_geoemtry.y, true);
        }

        window->resize_window_absolute(window->pre_state_change_geoemtry.width, window->pre_state_change_geoemtry.height, true);

        XStoreName(DISPLAY, window->get_frame(), "normal");
    }

    window->set_maximized(!window->is_maximized());
}

void _fullscreen_window(std::shared_ptr<EshyWMWindow> window, void* b_from_move_or_resize)
{
    if(!window->is_fullscreen())
    {
        if(window->is_maximized())
            _maximize_window(window, b_from_move_or_resize);
        
        TASKBAR->show_taskbar(false);
        window->set_show_titlebar(false);
        
        window->pre_state_change_geoemtry = window->get_frame_geometry();
        const int i = majority_monitor(window->get_frame_geometry());
        window->move_window_absolute(DISPLAY_X(i), DISPLAY_Y(i), true);
        window->resize_window_absolute(DISPLAY_WIDTH(i), DISPLAY_HEIGHT_RAW(i), true);

        XStoreName(DISPLAY, window->get_frame(), "fullscreen");
    }
    else
    {
        TASKBAR->show_taskbar(true);
        window->set_show_titlebar(true);

        if(!(bool*)(b_from_move_or_resize))
            window->move_window_absolute(window->pre_state_change_geoemtry.x, window->pre_state_change_geoemtry.y, true);

        window->resize_window_absolute(window->pre_state_change_geoemtry.width, window->pre_state_change_geoemtry.height, true);

        XStoreName(DISPLAY, window->get_frame(), "normal");
    }

    window->set_fullscreen(!window->is_fullscreen());
}

void _close_window(std::shared_ptr<EshyWMWindow> window, void* null)
{
    window->remove_grab_events();
    window->unframe_window();

    Atom* supported_protocols;
    int num_supported_protocols;

    static const Atom wm_delete_window = XInternAtom(DISPLAY, "WM_DELETE_WINDOW", false);

    if (XGetWMProtocols(DISPLAY, window->window, &supported_protocols, &num_supported_protocols) && (std::find(supported_protocols, supported_protocols + num_supported_protocols, wm_delete_window) != supported_protocols + num_supported_protocols))
    {
        static const Atom wm_protocols = XInternAtom(DISPLAY, "WM_PROTOCOLS", false);

        XEvent message;
        memset(&message, 0, sizeof(message));
        message.xclient.type = ClientMessage;
        message.xclient.message_type = wm_protocols;
        message.xclient.window = window->window;
        message.xclient.format = 32;
        message.xclient.data.l[0] = wm_delete_window;
        XSendEvent(DISPLAY, window->window, false, 0 , &message);
    }
    else
    {
        XKillClient(DISPLAY, window->window);
    }
}
}

void EshyWMWindow::frame_window(XWindowAttributes attr)
{
    frame_geometry = {attr.x, attr.y, (uint)attr.width, (uint)(attr.height + EshyWMConfig::titlebar_height)};
    frame = XCreateSimpleWindow(DISPLAY, ROOT, frame_geometry.x, frame_geometry.y, frame_geometry.width, frame_geometry.height, EshyWMConfig::window_frame_border_width, EshyWMConfig::window_frame_border_color, EshyWMConfig::window_background_color);
    XSelectInput(DISPLAY, frame, SubstructureNotifyMask | SubstructureRedirectMask);
    XReparentWindow(DISPLAY, window, frame, 0, (!b_force_no_titlebar ? EshyWMConfig::titlebar_height : 0));
    XMapWindow(DISPLAY, frame);

    //Set frame name to represent if the frame is normal, maximized, or fullscreen. Default is normal.
    XStoreName(DISPLAY, frame, "normal");

    //Set frame class to match the window
    Atom ATOM_CLASS = XInternAtom(DISPLAY, "WM_CLASS", False);

    Atom type;
    int format;
    unsigned long n_items;
    unsigned long bytes_after;
    unsigned char* property_value = NULL;
    int status = XGetWindowProperty(DISPLAY, window, ATOM_CLASS, 0, 1024, False, AnyPropertyType, &type, &format, &n_items, &bytes_after, &property_value);

    if(status == Success && type != None && format == 8)
        XChangeProperty(DISPLAY, frame, ATOM_CLASS, XA_STRING, 8, PropModeReplace, reinterpret_cast<const unsigned char*>(property_value), strlen(reinterpret_cast<const char*>(property_value)));

    XFree(property_value);
    XFlush(DISPLAY);

    if(!b_force_no_titlebar)
    {
        titlebar = XCreateSimpleWindow(DISPLAY, ROOT, attr.x, attr.y, attr.width, EshyWMConfig::titlebar_height, 0, EshyWMConfig::window_frame_border_color, EshyWMConfig::window_background_color);
        XSelectInput(DISPLAY, titlebar, VisibilityChangeMask);
        XReparentWindow(DISPLAY, titlebar, frame, 0, 0);
        XMapWindow(DISPLAY, titlebar);
        
        const rect initial_size = {0, 0, EshyWMConfig::titlebar_button_size, EshyWMConfig::titlebar_button_size};
        const button_color_data color = {EshyWMConfig::titlebar_button_normal_color, EshyWMConfig::titlebar_button_hovered_color, EshyWMConfig::titlebar_button_pressed_color};
        const button_color_data close_button_color = {EshyWMConfig::titlebar_button_normal_color, EshyWMConfig::close_button_color, EshyWMConfig::titlebar_button_pressed_color};
        minimize_button = std::make_shared<ImageButton>(titlebar, initial_size, color, "/home/eshy/.config/eshywm/minimize_window_icon.png");
        maximize_button = std::make_shared<ImageButton>(titlebar, initial_size, color, "/home/eshy/.config/eshywm/maximize_window_icon.png");
        close_button = std::make_shared<ImageButton>(titlebar, initial_size, close_button_color, "/home/eshy/.config/eshywm/close_window_icon.png");
        minimize_button->get_data() = {shared_from_this(), &WindowManager::_minimize_window};
        maximize_button->get_data() = {shared_from_this(), &WindowManager::_maximize_window};
        close_button->get_data() = {shared_from_this(), &WindowManager::_close_window};   
    }

    set_size_according_to(frame_geometry.width, frame_geometry.height);
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

void EshyWMWindow::setup_grab_events()
{
    //Basic movement and resizing
    XGrabButton(DISPLAY, Button1, AnyModifier, frame, false, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    XGrabButton(DISPLAY, Button1, Mod4Mask | ShiftMask, frame, false, ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(DISPLAY, Button3, Mod4Mask | ShiftMask, frame, false, ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(DISPLAY, Button1, AnyModifier, titlebar, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, titlebar, None);

    //Basic functions
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_c | XK_C), AnyModifier, frame, false, GrabModeAsync, GrabModeSync);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_d | XK_D), AnyModifier, frame, false, GrabModeAsync, GrabModeSync);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_f | XK_F), AnyModifier, frame, false, GrabModeAsync, GrabModeSync);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_a | XK_A), AnyModifier, frame, false, GrabModeAsync, GrabModeSync);
}

void EshyWMWindow::remove_grab_events()
{
    //Basic movement and resizing
    XUngrabButton(DISPLAY, Button1, AnyModifier, frame);
    XUngrabButton(DISPLAY, Button3, Mod1Mask, frame);
    XUngrabButton(DISPLAY, Button1, AnyModifier, titlebar);

    //Basic functions
    XUngrabButton(DISPLAY, XKeysymToKeycode(DISPLAY, XK_c | XK_C), AnyModifier, frame);
    XUngrabButton(DISPLAY, XKeysymToKeycode(DISPLAY, XK_f | XK_F), AnyModifier, frame);
    XUngrabButton(DISPLAY, XKeysymToKeycode(DISPLAY, XK_a | XK_A), AnyModifier, frame);
}


void EshyWMWindow::move_window_absolute(int new_position_x, int new_position_y, bool b_from_maximize)
{
    if((b_is_maximized || b_is_fullscreen) && !b_from_maximize)
    {
        WindowManager::_maximize_window(shared_from_this(), (void*)true);
    }

    frame_geometry.x = new_position_x;
    frame_geometry.y = new_position_y;
    XMoveWindow(DISPLAY, frame, new_position_x, new_position_y);
    update_titlebar();
}

void EshyWMWindow::resize_window_absolute(uint new_size_x, uint new_size_y, bool b_from_maximize)
{
    if((b_is_maximized || b_is_fullscreen) && !b_from_maximize)
    {
        WindowManager::_maximize_window(shared_from_this(), (void*)true);
    }

    set_size_according_to(new_size_x, new_size_y);
    XResizeWindow(DISPLAY, frame, frame_geometry.width, frame_geometry.height);
    XResizeWindow(DISPLAY, window, window_geometry.width, window_geometry.height);
    XResizeWindow(DISPLAY, titlebar, titlebar_geometry.width, titlebar_geometry.height);
    update_titlebar();
}

void EshyWMWindow::set_size_according_to(uint new_width, uint new_height)
{
    frame_geometry.width = window_geometry.width = titlebar_geometry.width = new_width;
    frame_geometry.height = new_height;

    window_geometry.height = new_height - (b_show_titlebar ? EshyWMConfig::titlebar_height : 0);
}

void EshyWMWindow::recalculate_all_window_size_and_location()
{
    Window return_window;
    int x;
    int y;
    unsigned int unsigned_null;
    unsigned int width;
    unsigned int height;

    XGetGeometry(DISPLAY, frame, &return_window, &x, &y, &width, &height, &unsigned_null, &unsigned_null);
    frame_geometry = {x, y, width, height};
    
    XGetGeometry(DISPLAY, window, &return_window, &x, &y, &width, &height, &unsigned_null, &unsigned_null);
    window_geometry = {x, y, width, height};

    XGetGeometry(DISPLAY, titlebar, &return_window, &x, &y, &width, &height, &unsigned_null, &unsigned_null);
    titlebar_geometry = {x, y, width, height};
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
        resize_window_absolute(frame_geometry.width, frame_geometry.height, false);
    }
    else
    {
        XUnmapWindow(DISPLAY, titlebar);
        XMoveWindow(DISPLAY, window, 0, 0);
        resize_window_absolute(frame_geometry.width, frame_geometry.height, false);
    }
}

void EshyWMWindow::update_titlebar()
{
    if(!b_show_titlebar)
        return;
    
    XClearWindow(DISPLAY, titlebar);

    XTextProperty name;
    XGetWMName(DISPLAY, window, &name);

    //Draw title
    cairo_titlebar_surface = cairo_xlib_surface_create(DISPLAY, titlebar, DefaultVisual(DISPLAY, 0), titlebar_geometry.width, titlebar_geometry.height);
    cairo_context = cairo_create(cairo_titlebar_surface);

    cairo_select_font_face(cairo_context, "Lato", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cairo_context, 16);
    cairo_set_source_rgb(cairo_context, 1.0f, 1.0f, 1.0f);
    cairo_move_to(cairo_context, 10.0f, 16.0f);
    cairo_show_text(cairo_context, reinterpret_cast<char*>(name.value));

    cairo_destroy(cairo_context);
    cairo_surface_destroy(cairo_titlebar_surface);

    const int button_y_offset = (EshyWMConfig::titlebar_height - EshyWMConfig::titlebar_button_size) / 2;
    const auto update_button_position = [this, button_y_offset](std::shared_ptr<Button> button, int i)
    {
        button->set_position((get_window_geometry().width - EshyWMConfig::titlebar_button_size * i) - (EshyWMConfig::titlebar_button_padding * (i - 1)) - button_y_offset, button_y_offset);
        button->draw();
    };

    update_button_position(minimize_button, 3);
    update_button_position(maximize_button, 2);
    update_button_position(close_button, 1);
}