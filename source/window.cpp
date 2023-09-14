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

#include <algorithm>
#include <cstring>
#include <fstream>

#define GRAB_KEY(key, main_modifier, window) \
    XGrabKey(DISPLAY, key, main_modifier, window, false, GrabModeAsync, GrabModeAsync); \
    XGrabKey(DISPLAY, key, main_modifier | Mod2Mask, window, false, GrabModeAsync, GrabModeAsync); \
    XGrabKey(DISPLAY, key, main_modifier | LockMask, window, false, GrabModeAsync, GrabModeAsync); \
    XGrabKey(DISPLAY, key, main_modifier | Mod2Mask | LockMask, window, false, GrabModeAsync, GrabModeAsync);

#define UNGRAB_KEY(key, main_modifier, window) \
    XUngrabKey(DISPLAY, key, main_modifier, window); \
    XUngrabKey(DISPLAY, key, main_modifier | Mod2Mask, window); \
    XUngrabKey(DISPLAY, key, main_modifier | LockMask, window); \
    XUngrabKey(DISPLAY, key, main_modifier | Mod2Mask | LockMask, window);

#define GRAB_BUTTON(button, main_modifier, window, masks) \
    XGrabButton(DISPLAY, button, main_modifier, window, false, masks, GrabModeAsync, GrabModeAsync, None, None); \
    XGrabButton(DISPLAY, button, main_modifier | Mod2Mask, window, false, masks, GrabModeAsync, GrabModeAsync, None, None); \
    XGrabButton(DISPLAY, button, main_modifier | LockMask, window, false, masks, GrabModeAsync, GrabModeAsync, None, None); \
    XGrabButton(DISPLAY, button, main_modifier | Mod2Mask | LockMask, window, false, masks, GrabModeAsync, GrabModeAsync, None, None);

#define UNGRAB_BUTTON(button, main_modifier, window) \
    XUngrabButton(DISPLAY, button, main_modifier, window); \
    XUngrabButton(DISPLAY, button, main_modifier | Mod2Mask, window); \
    XUngrabButton(DISPLAY, button, main_modifier | LockMask, window); \
    XUngrabButton(DISPLAY, button, main_modifier | Mod2Mask | LockMask, window);

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

EshyWMWindow::EshyWMWindow(Window _window, bool _b_force_no_titlebar)
    : window(_window)
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
    pre_state_change_geometry = frame_geometry = {attr.x, attr.y, (uint)attr.width, (uint)(attr.height + EshyWMConfig::titlebar_height)};
    frame = XCreateSimpleWindow(DISPLAY, ROOT, frame_geometry.x, frame_geometry.y, frame_geometry.width, frame_geometry.height, EshyWMConfig::window_frame_border_width, EshyWMConfig::window_frame_border_color, EshyWMConfig::window_background_color);
    XSelectInput(DISPLAY, frame, SubstructureNotifyMask | SubstructureRedirectMask | EnterWindowMask);
    //Resize because in case we use the * 0.9 version of height, then the bottom is cut off
    XResizeWindow(DISPLAY, window, attr.width, attr.height);
    XReparentWindow(DISPLAY, window, frame, 0, (!b_force_no_titlebar ? EshyWMConfig::titlebar_height : 0));
    XMapWindow(DISPLAY, frame);

    majority_monitor(frame_geometry, current_monitor);
    set_window_state(WS_NORMAL);

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
        titlebar = XCreateSimpleWindow(DISPLAY, frame, 0, 0, attr.width, EshyWMConfig::titlebar_height, 0, EshyWMConfig::window_frame_border_color, EshyWMConfig::window_background_color);
        XSelectInput(DISPLAY, titlebar, VisibilityChangeMask);
        XMapWindow(DISPLAY, titlebar);

        cairo_titlebar_surface = cairo_xlib_surface_create(DISPLAY, titlebar, DefaultVisual(DISPLAY, 0), attr.width, EshyWMConfig::titlebar_height);
        cairo_context = cairo_create(cairo_titlebar_surface);
        
        const rect initial_size = {0, 0, EshyWMConfig::titlebar_button_size, EshyWMConfig::titlebar_button_size};
        const button_color_data color = {EshyWMConfig::titlebar_button_normal_color, EshyWMConfig::titlebar_button_hovered_color, EshyWMConfig::titlebar_button_pressed_color};
        const button_color_data close_button_color = {EshyWMConfig::titlebar_button_normal_color, EshyWMConfig::close_button_color, EshyWMConfig::titlebar_button_pressed_color};
        minimize_button = std::make_shared<ImageButton>(titlebar, initial_size, color, EshyWMConfig::minimize_button_image_path.c_str());
        maximize_button = std::make_shared<ImageButton>(titlebar, initial_size, color, EshyWMConfig::maximize_button_image_path.c_str());
        close_button = std::make_shared<ImageButton>(titlebar, initial_size, close_button_color, EshyWMConfig::close_button_image_path.c_str());
        minimize_button->get_data() = {shared_from_this(), &EshyWMWindow::toggle_minimize};
        maximize_button->get_data() = {shared_from_this(), &EshyWMWindow::toggle_maximize};
        close_button->get_data() = {shared_from_this(), &EshyWMWindow::close_window};
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

void EshyWMWindow::setup_grab_events()
{
    //Basic movement and resizing
    XGrabButton(DISPLAY, Button1, AnyModifier, frame, false, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    GRAB_BUTTON(Button1, Mod4Mask, frame, ButtonReleaseMask | ButtonMotionMask);
    GRAB_BUTTON(Button3, Mod4Mask, frame, ButtonReleaseMask | ButtonMotionMask);
    XGrabButton(DISPLAY, Button1, AnyModifier, titlebar, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, titlebar, None);

    //Basic functions
    GRAB_KEY(XKeysymToKeycode(DISPLAY, XK_c | XK_C), Mod4Mask, frame);
    GRAB_KEY(XKeysymToKeycode(DISPLAY, XK_d | XK_D), Mod4Mask, frame);
    GRAB_KEY(XKeysymToKeycode(DISPLAY, XK_f | XK_F), Mod4Mask, frame);
    GRAB_KEY(XKeysymToKeycode(DISPLAY, XK_a | XK_A), Mod4Mask, frame);

    GRAB_KEY(XKeysymToKeycode(DISPLAY, XK_Left), Mod4Mask, frame);
    GRAB_KEY(XKeysymToKeycode(DISPLAY, XK_Up), Mod4Mask, frame);
    GRAB_KEY(XKeysymToKeycode(DISPLAY, XK_Right), Mod4Mask, frame);
    GRAB_KEY(XKeysymToKeycode(DISPLAY, XK_Down), Mod4Mask, frame);

    GRAB_KEY(XKeysymToKeycode(DISPLAY, XK_Left), Mod4Mask | ShiftMask, frame);
    GRAB_KEY(XKeysymToKeycode(DISPLAY, XK_Up), Mod4Mask | ShiftMask, frame);
    GRAB_KEY(XKeysymToKeycode(DISPLAY, XK_Right), Mod4Mask | ShiftMask, frame);
    GRAB_KEY(XKeysymToKeycode(DISPLAY, XK_Down), Mod4Mask | ShiftMask, frame);

    GRAB_KEY(XKeysymToKeycode(DISPLAY, XK_o | XK_O), Mod4Mask, frame);
    GRAB_KEY(XKeysymToKeycode(DISPLAY, XK_i | XK_I), Mod4Mask, frame);
}

void EshyWMWindow::remove_grab_events()
{
    //Basic movement and resizing
    XUngrabButton(DISPLAY, Button1, AnyModifier, frame);
    UNGRAB_BUTTON(Button1, Mod4Mask, frame);
    UNGRAB_BUTTON(Button3, Mod4Mask, frame);
    XUngrabButton(DISPLAY, Button1, AnyModifier, titlebar);

    //Basic functions
    UNGRAB_KEY(XKeysymToKeycode(DISPLAY, XK_c | XK_C), Mod4Mask, frame);
    UNGRAB_KEY(XKeysymToKeycode(DISPLAY, XK_d | XK_D), Mod4Mask, frame);
    UNGRAB_KEY(XKeysymToKeycode(DISPLAY, XK_f | XK_F), Mod4Mask, frame);
    UNGRAB_KEY(XKeysymToKeycode(DISPLAY, XK_a | XK_A), Mod4Mask, frame);

    UNGRAB_KEY(XKeysymToKeycode(DISPLAY, XK_Left), Mod4Mask, frame);
    UNGRAB_KEY(XKeysymToKeycode(DISPLAY, XK_Up), Mod4Mask, frame);
    UNGRAB_KEY(XKeysymToKeycode(DISPLAY, XK_Right), Mod4Mask, frame);
    UNGRAB_KEY(XKeysymToKeycode(DISPLAY, XK_Down), Mod4Mask, frame);

    UNGRAB_KEY(XKeysymToKeycode(DISPLAY, XK_o | XK_O), Mod4Mask, frame);
    UNGRAB_KEY(XKeysymToKeycode(DISPLAY, XK_i | XK_I), Mod4Mask, frame);
}


void EshyWMWindow::minimize_window(bool b_minimize, EWindowStateChangeCondition condition)
{
    if(b_minimize)
    {
        XUnmapWindow(DISPLAY, get_frame());
        if(window_state == WS_FULLSCREEN)
        {
            move_window_absolute(pre_state_change_geometry.x, pre_state_change_geometry.y);
            resize_window_absolute(pre_state_change_geometry.width, pre_state_change_geometry.height);
        }
        set_window_state(WS_MINIMIZED);
    }
    else
    {
        XMapWindow(DISPLAY, get_frame());
        update_titlebar();
        set_window_state(WS_NORMAL);
    }
}

void EshyWMWindow::maximize_window(bool b_maximize, EWindowStateChangeCondition condition)
{
    if(b_maximize)
    {
        std::shared_ptr<s_monitor_info> monitor;
        if(majority_monitor(frame_geometry, monitor))
        {
            if(window_state == WS_FULLSCREEN)
                fullscreen_window(false, WSCC_FROM_MAXIMIZE);
            else if (window_state == WS_NORMAL)
                pre_state_change_geometry = frame_geometry;

            move_window_absolute(monitor->x, monitor->y, true);
            resize_window_absolute(monitor->width - (EshyWMConfig::window_frame_border_width * 2), (monitor->height - EshyWMConfig::taskbar_height) - (EshyWMConfig::window_frame_border_width * 2), true);

            set_window_state(WS_MAXIMIZED);
        }
    }
    else
    {
        if(condition <= WSCC_FROM_RESIZE)
        {
            if(condition != WSCC_FROM_MOVE)
                move_window_absolute(pre_state_change_geometry.x, pre_state_change_geometry.y, true);
            if(condition != WSCC_FROM_RESIZE)
                resize_window_absolute(pre_state_change_geometry.width, pre_state_change_geometry.height, true);   
        }

        set_window_state(WS_NORMAL);
    }
}

void EshyWMWindow::fullscreen_window(bool b_fullscreen, EWindowStateChangeCondition condition)
{
    if(b_fullscreen)
    {        
        std::shared_ptr<s_monitor_info> monitor;
        if(majority_monitor(frame_geometry, monitor))
        {
            if (window_state == WS_NORMAL)
                pre_state_change_geometry = frame_geometry;
            if(condition == WSCC_STORE_STATE)
                previous_state = window_state;
        
            set_show_titlebar(false);

            current_monitor = monitor;
            monitor->taskbar->show_taskbar(false);
            move_window_absolute(monitor->x, monitor->y, true);
            resize_window_absolute(monitor->width, monitor->height, true);

            set_window_state(WS_FULLSCREEN);
        }
    }
    else
    {
        if(previous_state == WS_MAXIMIZED)
        {
            previous_state = WS_NONE;
            maximize_window(true);
            return;
        }

        if(current_monitor)
            current_monitor->taskbar->show_taskbar(true);

        set_show_titlebar(true);

        if(condition <= WSCC_FROM_RESIZE)
        {
            if(condition != WSCC_FROM_MOVE)
                move_window_absolute(pre_state_change_geometry.x, pre_state_change_geometry.y, true);
            if(condition != WSCC_FROM_RESIZE)
                resize_window_absolute(pre_state_change_geometry.width, pre_state_change_geometry.height, true);
        }

        set_window_state(WS_NORMAL);
    }
}

void EshyWMWindow::close_window(std::shared_ptr<EshyWMWindow> window, void* null)
{
    XUnmapWindow(DISPLAY, window->window);
    
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

void EshyWMWindow::focus_window(std::shared_ptr<class EshyWMWindow> window)
{
    XSetInputFocus(DISPLAY, window->get_window(), RevertToPointerRoot, CurrentTime);

    auto i = std::find(WindowManager::window_list.begin(), WindowManager::window_list.end(), window);
    auto it = WindowManager::window_list.begin() + std::distance(WindowManager::window_list.begin(), i);
    std::rotate(WindowManager::window_list.begin(), it, it + 1);

    SWITCHER->update_switcher_window_options();
}

void EshyWMWindow::anchor_window(EWindowState anchor, std::shared_ptr<s_monitor_info> monitor_override)
{
    if(window_state == anchor)
    {
        attempt_shift_monitor_anchor(anchor);
        return;
    }

    std::shared_ptr<s_monitor_info> monitor_info = monitor_override;
    if(!monitor_override && !majority_monitor(get_frame_geometry(), monitor_info))
        return;
    
    if(window_state == WS_NORMAL)
        pre_state_change_geometry = frame_geometry;

    switch (anchor)
    {
    case WS_ANCHORED_LEFT:
    {
        if (window_state == WS_ANCHORED_RIGHT && !monitor_override) goto set_normal;
        move_window_absolute(monitor_info->x, monitor_info->y);
        resize_window_absolute(monitor_info->width / 2, monitor_info->height - EshyWMConfig::taskbar_height);
        break;
    }
    case WS_ANCHORED_UP:
    {
        if (window_state == WS_ANCHORED_DOWN && !monitor_override) goto set_normal;
        move_window_absolute(monitor_info->x, monitor_info->y);
        resize_window_absolute(monitor_info->width, (monitor_info->height - EshyWMConfig::taskbar_height) / 2);
        break;
    }
    case WS_ANCHORED_RIGHT:
    {
        if (window_state == WS_ANCHORED_LEFT && !monitor_override) goto set_normal;
        move_window_absolute(monitor_info->x + (monitor_info->width / 2), monitor_info->y);
        resize_window_absolute(monitor_info->width / 2, monitor_info->height - EshyWMConfig::taskbar_height);
        break;
    }
    case WS_ANCHORED_DOWN:
    {
        if (window_state == WS_ANCHORED_UP && !monitor_override) goto set_normal;
        move_window_absolute(monitor_info->x, monitor_info->y + ((monitor_info->height - EshyWMConfig::taskbar_height) / 2));
        resize_window_absolute(monitor_info->width, (monitor_info->height - EshyWMConfig::taskbar_height) / 2);
        break;
    }
    case WS_NORMAL:
    {
set_normal:
        set_window_state(WS_NORMAL);
        move_window_absolute(pre_state_change_geometry.x, pre_state_change_geometry.y);
        resize_window_absolute(pre_state_change_geometry.width, pre_state_change_geometry.height);
        return;
    }
    };

    set_window_state(anchor);
}

void EshyWMWindow::attempt_shift_monitor_anchor(EWindowState direction)
{
    std::shared_ptr<s_monitor_info> monitor_info;
    if(!majority_monitor(get_frame_geometry(), monitor_info))
        return;

    int test_x = 0;
    int test_y = 0;
    EWindowState anchor = WS_NONE;
    rect new_pre_state_change_geometry = pre_state_change_geometry;

    switch(direction)
    {
    case WS_ANCHORED_LEFT:
    {
        test_x = monitor_info->x - 10;
        test_y = monitor_info->y;
        anchor = WS_ANCHORED_RIGHT;
        new_pre_state_change_geometry.x -= current_monitor->width;
        break;
    }
    case WS_ANCHORED_UP:
    {
        test_x = monitor_info->x;
        test_y = monitor_info->y - 10;
        anchor = WS_ANCHORED_DOWN;
        new_pre_state_change_geometry.y -= current_monitor->height;
        break;
    }
    case WS_ANCHORED_RIGHT:
    {
        test_x = monitor_info->x + monitor_info->width + 10;
        test_y = monitor_info->y;
        anchor = WS_ANCHORED_LEFT;
        new_pre_state_change_geometry.x += current_monitor->width;
        break;
    }
    case WS_ANCHORED_DOWN:
    {
        test_x = monitor_info->x;
        test_y = monitor_info->y + monitor_info->height + 10;
        anchor = WS_ANCHORED_UP;
        new_pre_state_change_geometry.y += current_monitor->height;
        break;
    }
    };

    std::shared_ptr<s_monitor_info> new_monitor_info;
    if(position_in_monitor(test_x, test_y, new_monitor_info))
    {
        pre_state_change_geometry = new_pre_state_change_geometry;
        pre_state_change_geometry.width *= current_monitor->width / new_monitor_info->width;
        pre_state_change_geometry.height *= current_monitor->height / new_monitor_info->height;
        anchor_window(anchor, new_monitor_info);
        current_monitor = new_monitor_info;
    }
}

void EshyWMWindow::attempt_shift_monitor(EWindowState direction)
{
    std::shared_ptr<s_monitor_info> monitor_info;
    if(!majority_monitor(get_frame_geometry(), monitor_info))
        return;

    int test_x = 0;
    int test_y = 0;
    rect new_pre_state_change_geometry = pre_state_change_geometry;

    switch(direction)
    {
    case WS_ANCHORED_LEFT:
    {
        test_x = monitor_info->x - 10;
        test_y = monitor_info->y;
        new_pre_state_change_geometry.x -= current_monitor->width;
        break;
    }
    case WS_ANCHORED_UP:
    {
        test_x = monitor_info->x;
        test_y = monitor_info->y - 10;
        new_pre_state_change_geometry.y -= current_monitor->height;
        break;
    }
    case WS_ANCHORED_RIGHT:
    {
        test_x = monitor_info->x + monitor_info->width + 10;
        test_y = monitor_info->y;
        new_pre_state_change_geometry.x += current_monitor->width;
        break;
    }
    case WS_ANCHORED_DOWN:
    {
        test_x = monitor_info->x;
        test_y = monitor_info->y + monitor_info->height + 10;
        new_pre_state_change_geometry.y += current_monitor->height;
        break;
    }
    };

    std::shared_ptr<s_monitor_info> new_monitor_info;
    if(position_in_monitor(test_x, test_y, new_monitor_info))
    {
        pre_state_change_geometry = new_pre_state_change_geometry;
        pre_state_change_geometry.width *= current_monitor->width / new_monitor_info->width;
        pre_state_change_geometry.height *= current_monitor->height / new_monitor_info->height;
        move_window_absolute(pre_state_change_geometry.x, pre_state_change_geometry.y);
        resize_window_absolute(pre_state_change_geometry.width, pre_state_change_geometry.height);
        current_monitor = new_monitor_info;
    }
}


void EshyWMWindow::move_window_absolute(int new_position_x, int new_position_y, bool b_from_maximize)
{
    if(!b_from_maximize)
    {
        if(window_state == WS_MAXIMIZED)
            maximize_window(false, WSCC_FROM_MOVE);
        else if(window_state == WS_FULLSCREEN)
            fullscreen_window(false, WSCC_FROM_MOVE);
    }

    if(window_state >= WS_ANCHORED_LEFT)
        anchor_window(WS_NORMAL);

    frame_geometry.x = new_position_x;
    frame_geometry.y = new_position_y;
    XMoveWindow(DISPLAY, frame, new_position_x, new_position_y);
}

void EshyWMWindow::resize_window_absolute(uint new_size_x, uint new_size_y, bool b_from_maximize)
{
    if(!b_from_maximize)
    {
        if(window_state == WS_MAXIMIZED)
            maximize_window(false, WSCC_FROM_RESIZE);
        else if(window_state == WS_FULLSCREEN)
            fullscreen_window(false, WSCC_FROM_RESIZE);
    }

    frame_geometry.width = new_size_x;
    frame_geometry.height = new_size_y;

    XResizeWindow(DISPLAY, frame, frame_geometry.width, frame_geometry.height);
    XResizeWindow(DISPLAY, window, new_size_x, new_size_y - (b_show_titlebar ? EshyWMConfig::titlebar_height : 0));
    XResizeWindow(DISPLAY, titlebar, new_size_x, EshyWMConfig::titlebar_height);
    update_titlebar();
}


void EshyWMWindow::set_window_state(EWindowState new_window_state)
{
    char new_state_name[15] = "";
    switch (new_window_state)
    {
    case WS_NORMAL:
        strcpy(new_state_name, "normal");
        break;
    case WS_MINIMIZED:
        strcpy(new_state_name, "minimized");
        break;
    case WS_MAXIMIZED:
        strcpy(new_state_name, "maximized");
        break;
    case WS_FULLSCREEN:
        strcpy(new_state_name, "fullscreen");
        break;
    case WS_ANCHORED_LEFT:
        strcpy(new_state_name, "anchored_left");
        break;
    case WS_ANCHORED_UP:
        strcpy(new_state_name, "anchored_up");
        break;
    case WS_ANCHORED_RIGHT:
        strcpy(new_state_name, "anchored_right");
        break;
    case WS_ANCHORED_DOWN:
        strcpy(new_state_name, "anchored_down");
        break;
    };
    
    XStoreName(DISPLAY, frame, new_state_name);
    window_state = new_window_state;
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
    if(!b_show_titlebar || !cairo_context)
        return;
    
    XClearWindow(DISPLAY, titlebar);

    XTextProperty name;
    XGetWMName(DISPLAY, window, &name);

    //Draw title
    cairo_select_font_face(cairo_context, "Lato", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cairo_context, 16);
    cairo_set_source_rgb(cairo_context, 1.0f, 1.0f, 1.0f);
    cairo_move_to(cairo_context, 10.0f, 16.0f);
    cairo_show_text(cairo_context, reinterpret_cast<char*>(name.value));

    const int button_y_offset = (EshyWMConfig::titlebar_height - EshyWMConfig::titlebar_button_size) / 2;
    const auto update_button_position = [this, button_y_offset](std::shared_ptr<Button> button, int i)
    {
        button->set_position((frame_geometry.width - EshyWMConfig::titlebar_button_size * i) - (EshyWMConfig::titlebar_button_padding * (i - 1)) - button_y_offset, button_y_offset);
        button->draw();
    };

    update_button_position(minimize_button, 3);
    update_button_position(maximize_button, 2);
    update_button_position(close_button, 1);
}
