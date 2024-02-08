#include "window_manager.h"
#include "eshywm.h"
#include "window.h"
#include "config.h"
#include "context_menu.h"
#include "taskbar.h"
#include "switcher.h"
#include "run_menu.h"
#include "button.h"

#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#include <cstring>
#include <algorithm>
#include <fstream>

#define XC_left_ptr 68

#define CHECK_KEYSYM_PRESSED(event, key_sym)                    if(event.keycode == XKeysymToKeycode(DISPLAY, key_sym))
#define ELSE_CHECK_KEYSYM_PRESSED(event, key_sym)               else if(event.keycode == XKeysymToKeycode(DISPLAY, key_sym))
#define CHECK_KEYSYM_AND_MOD_PRESSED(event, mod, key_sym)       if(event.state & mod && event.keycode == XKeysymToKeycode(DISPLAY, key_sym))
#define ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, mod, key_sym)  else if(event.state & mod && event.keycode == XKeysymToKeycode(DISPLAY, key_sym))

using namespace std::chrono_literals;

Display* WindowManager::display = nullptr;

void WindowManager::handle_button_hovered(Window hovered_window, bool b_hovered, int mode)
{
    if (currently_hovered_button)
    {
        //Going from hovered to normal
        if (currently_hovered_button->get_button_state() == EButtonState::S_Hovered && !b_hovered && mode == NotifyNormal)
        {
            currently_hovered_button->set_button_state(EButtonState::S_Normal);
            currently_hovered_button = nullptr;
        }
        //Going from hovered to pressed
        else if (currently_hovered_button->get_button_state() == EButtonState::S_Hovered && !b_hovered && mode == NotifyGrab)
            currently_hovered_button->set_button_state(EButtonState::S_Pressed);
        //Going from pressed to hovered
        else if (currently_hovered_button->get_button_state() == EButtonState::S_Pressed && b_hovered && mode == NotifyUngrab)
            currently_hovered_button->set_button_state(EButtonState::S_Hovered);
    }

    //Going from normal to hovered (need extra logic because our currently_hovered_button will change here)
    if (((!currently_hovered_button) || (currently_hovered_button && currently_hovered_button->get_button_state() == EButtonState::S_Normal)) && b_hovered)
    {
        if(currently_hovered_button)
            currently_hovered_button->set_button_state(EButtonState::S_Normal);

        for(std::shared_ptr<s_monitor_info> monitor : EshyWM::monitors)
            for(std::shared_ptr<WindowButton>& button : monitor->taskbar->get_taskbar_buttons())
            {
                if(button->get_window() == hovered_window)
                {
                    currently_hovered_button = button;
                    break;
                }
            }

        if(!currently_hovered_button)
            for(std::shared_ptr<EshyWMWindow>& window : window_list)
            {
                if(window->get_minimize_button() && window->get_minimize_button()->get_window() == hovered_window)
                    currently_hovered_button = window->get_minimize_button();
                else if(window->get_maximize_button() && window->get_maximize_button()->get_window() == hovered_window)
                    currently_hovered_button = window->get_maximize_button();
                else if(window->get_close_button() && window->get_close_button()->get_window() == hovered_window)
                    currently_hovered_button = window->get_close_button();

                if(currently_hovered_button)
                    break;
            }

        if(currently_hovered_button)
            currently_hovered_button->set_button_state(EButtonState::S_Hovered);
    }
}


void WindowManager::initialize()
{
    display = XOpenDisplay(nullptr);
    ensure(display)

    XSetErrorHandler([](Display* display, XErrorEvent* event)
    {
        ensure((int)(event->error_code) == BadAccess)
        abort();
        return 0;
    });
    
    XSelectInput(DISPLAY, ROOT, PointerMotionMask | SubstructureRedirectMask | StructureNotifyMask | SubstructureNotifyMask);
    XSync(DISPLAY, false);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_e | XK_E), AnyModifier, ROOT, false, GrabModeAsync, GrabModeSync);

    //Grab all the keybound keys
    for(const EshyWMConfig::KeyBinding key_binding : EshyWMConfig::key_bindings)
        XGrabKey(DISPLAY, key_binding.key, AnyModifier, ROOT, false, GrabModeAsync, GrabModeSync);

    XDefineCursor(DISPLAY, ROOT, XCreateFontCursor(DISPLAY, XC_left_ptr));
    
    EshyWM::update_monitor_info();
    titlebar_double_click = {NULL, 0, 0};
}

void WindowManager::handle_events() 
{
    static XEvent event;

    while(XEventsQueued(DISPLAY, QueuedAfterFlush) != 0)
    {
        std::cout << "KEYCODE FOR F6:" << XKeysymToKeycode(DISPLAY, XK_F6) << std::endl;

        XNextEvent(DISPLAY, &event);
        //LOG_EVENT_INFO(LS_Verbose, event)
        
        switch (event.type)
        {
        case DestroyNotify:
            OnDestroyNotify(event.xdestroywindow);
            break;
        case MapRequest:
            OnMapRequest(event.xmaprequest);
            break;
        case MapNotify:
            OnMapNotify(event.xmap);
            break;
        case UnmapNotify:
            OnUnmapNotify(event.xunmap);
            break;
        case PropertyNotify:
            OnPropertyNotify(event.xproperty);
            break;
        case ConfigureNotify:
            OnConfigureNotify(event.xconfigure);
            break;
        case ConfigureRequest:
            OnConfigureRequest(event.xconfigurerequest);
            break;
        case VisibilityNotify:
            OnVisibilityNotify(event.xvisibility);
            break;
        case ButtonPress:
            OnButtonPress(event.xbutton);
            break;
        case ButtonRelease:
            OnButtonRelease(event.xbutton);
            break;
        case MotionNotify:
            while (XCheckTypedWindowEvent(DISPLAY, event.xmotion.window, MotionNotify, &event)) {}
            OnMotionNotify(event.xmotion);
            break;
        case KeyPress:
            OnKeyPress(event.xkey);
            break;
        case KeyRelease:
            OnKeyRelease(event.xkey);
            break;
        case EnterNotify:
            OnEnterNotify(event.xcrossing);
            break;
        case LeaveNotify:
            if(event.xcrossing.window != ROOT)
                handle_button_hovered(event.xcrossing.window, false, event.xcrossing.mode);
            break;
        case ClientMessage:
            OnClientMessage(event.xclient);
            break;
        };
    }
}

void WindowManager::handle_preexisting_windows()
{
    XSetErrorHandler([](Display* display, XErrorEvent* event)
    {
        const int MAX_ERROR_TEXT_LEGTH = 1024;
        char error_text[MAX_ERROR_TEXT_LEGTH];
        XGetErrorText(display, event->error_code, error_text, sizeof(error_text));
        return 0;
    });

    XGrabServer(DISPLAY);

    Window returned_root;
    Window returned_parent;
    Window* top_level_windows;
    unsigned int num_top_level_windows;
    XQueryTree(DISPLAY, ROOT, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows);

    for(unsigned int i = 0; i < num_top_level_windows; ++i)
    {
        bool b_continue = false;
        for(std::shared_ptr<s_monitor_info> monitor : EshyWM::monitors)
            if(monitor->taskbar->get_menu_window() == top_level_windows[i])
            {
                b_continue = true;
                break;
            }

        if(b_continue)
            continue;

        if(top_level_windows[i] != SWITCHER->get_menu_window()
        && top_level_windows[i] != CONTEXT_MENU->get_menu_window())
        {
            register_window(top_level_windows[i], true);
        }
    }

    XFree(top_level_windows);
    XUngrabServer(DISPLAY);
}


std::shared_ptr<EshyWMWindow> WindowManager::register_window(Window window, bool b_was_created_before_window_manager)
{
    //Do not frame already framed windows
    if(window_list_contains_window(window))
        return nullptr;
    
    //Retrieve attributes of window to frame
    XWindowAttributes x_window_attributes = {0};
    XGetWindowAttributes(DISPLAY, window, &x_window_attributes);

    //If window was created before window manager started, we should frame it only if it is visible and does not set override_redirect
    if(b_was_created_before_window_manager && (x_window_attributes.override_redirect || x_window_attributes.map_state != IsViewable))
        return nullptr;

    //Add so we can restore if we crash
    XAddToSaveSet(DISPLAY, window);

    //Center window, when creating we want to center it on the monitor the cursor is in
    Window window_return;
    int root_x;
    int root_y;
    int others;
    uint mask_return;

    XQueryPointer(DISPLAY, ROOT, &window_return, &window_return, &root_x, &root_y, &others, &others, &mask_return);

    //If window was previously maximized when it was closed, then maximize again. Otherwise center and clamp size
    Atom ATOM_CLASS = XInternAtom(DISPLAY, "WM_CLASS", False);

    Atom type;
    int format;
    unsigned long n_items;
    unsigned long bytes_after;
    unsigned char* property_value = NULL;
    int status = XGetWindowProperty(DISPLAY, window, ATOM_CLASS, 0, 1024, False, AnyPropertyType, &type, &format, &n_items, &bytes_after, &property_value);

    const std::string window_name = std::string(reinterpret_cast<const char*>(property_value));
    const bool b_begin_maximized = EshyWMConfig::window_close_data.find(window_name) != EshyWMConfig::window_close_data.end() && EshyWMConfig::window_close_data[window_name] == "maximized";

    std::shared_ptr<s_monitor_info> monitor;
    if(position_in_monitor(root_x, root_y, monitor))
    {
        x_window_attributes.width = std::min((int)(monitor->width * 0.9f), x_window_attributes.width);
        x_window_attributes.height = std::min((int)((monitor->height - EshyWMConfig::taskbar_height) * 0.9f), x_window_attributes.height);

        //Set the x, y positions to be the center location
        x_window_attributes.x = std::clamp(CENTER_W(monitor, x_window_attributes.width), monitor->x, CENTER_W(monitor, 0));
        x_window_attributes.y = std::clamp(CENTER_H(monitor, x_window_attributes.height), monitor->y, CENTER_H(monitor, 0));
    }

    //Create window
    std::shared_ptr<EshyWMWindow> new_window = std::make_shared<EshyWMWindow>(window, false);
    new_window->frame_window(x_window_attributes);
    new_window->setup_grab_events();
    if(b_begin_maximized)
        new_window->maximize_window(true, WSCC_MANUAL);
    window_list.push_back(new_window);

    EshyWM::window_created_notify(new_window);
    return new_window;
}

void WindowManager::focus_window(std::shared_ptr<EshyWMWindow> window)
{
    XSetInputFocus(DISPLAY, window->get_window(), RevertToNone, CurrentTime);

    auto i = std::find(window_list.begin(), window_list.end(), window);
    auto it = window_list.begin() + std::distance(window_list.begin(), i);
    std::rotate(window_list.begin(), it, it + 1);

    SWITCHER->update_switcher_window_options();
}


void WindowManager::OnDestroyNotify(const XDestroyWindowEvent& event)
{
    if(std::shared_ptr<EshyWMWindow> window = window_list_contains_frame(event.window))
    {
        EshyWM::window_destroyed_notify(window);
        remove_from_window_list(window);

        //Send focus to last focused window
        if (window_list.size() && window_list[0])
           XSetInputFocus(DISPLAY, window_list[0]->get_window(), RevertToNone, CurrentTime);
        else
            XSetInputFocus(DISPLAY, ROOT, RevertToNone, CurrentTime);
    }
}

void WindowManager::OnMapNotify(const XMapEvent& event)
{
    //If the user has mapped a window, then we clear the view_desktop_window_list as it no longer applies
    view_desktop_window_list.clear();
}

void WindowManager::OnUnmapNotify(const XUnmapEvent& event)
{
    safe_ensure(event.event != ROOT);

    /**If we manage window then unframe. Need to check
     * because we will receive an UnmapNotify event for
     * a frame window we just destroyed. Ignore event
     * if it is triggered by reparenting a window that
     * was mapped before the window manager started*/
    if(std::shared_ptr<EshyWMWindow> window = window_list_contains_frame(event.window))
    {
        window->unframe_window();
    }
    else if(std::shared_ptr<EshyWMWindow> window = window_list_contains_window(event.window))
    {
        window->unframe_window();
    }

    //If all windows have been minimized, set input to root
    bool b_all_minimized = true;
    for(std::shared_ptr<EshyWMWindow> window : window_list)
        if(window->get_window_state() != WS_MINIMIZED)
        {
            b_all_minimized = false;
            break;
        }
    
    if(b_all_minimized || window_list.size() == 0)
        XSetInputFocus(DISPLAY, ROOT, RevertToNone, CurrentTime);
}

void WindowManager::OnMapRequest(const XMapRequestEvent& event)
{    
    register_window(event.window, false);
    XMapWindow(DISPLAY, event.window);
    XSetInputFocus(DISPLAY, event.window, RevertToNone, CurrentTime);
}

void WindowManager::OnPropertyNotify(const XPropertyEvent& event)
{
    if(std::shared_ptr<EshyWMWindow> window = window_list_contains_window(event.window))
        window->update_titlebar();
}

void WindowManager::OnConfigureNotify(const XConfigureEvent& event)
{
    if(event.window == ROOT && event.display == DISPLAY)
    {
        EshyWM::update_monitor_info();

        //Notify screen resolution changed
        EshyWM::on_screen_resolution_changed(event.width, event.height);
    }
}

void WindowManager::OnConfigureRequest(const XConfigureRequestEvent& event)
{
    XWindowChanges changes;
    changes.x = event.x;
    changes.y = event.y;
    changes.width = event.width;
    changes.height = event.height;
    changes.border_width = event.border_width;
    changes.sibling = event.above;
    changes.stack_mode = event.detail;

    if (std::shared_ptr<EshyWMWindow> window = window_list_contains_window(event.window))
    {
        //If inner window is moving than move frame to represent that movement instead
        changes.y -= EshyWMConfig::titlebar_height;
        XConfigureWindow(DISPLAY, window->get_frame(), event.value_mask, &changes);
    }
    else
        XConfigureWindow(DISPLAY, event.window, event.value_mask, &changes);
}

void WindowManager::OnVisibilityNotify(const XVisibilityEvent& event)
{
    for(std::shared_ptr<s_monitor_info> monitor : EshyWM::monitors)
        if(monitor->taskbar)
        {
            if(event.window == monitor->taskbar->get_menu_window())
                monitor->taskbar->raise();
            else
            {
                for(const std::shared_ptr<WindowButton>& button : monitor->taskbar->get_taskbar_buttons())
                {
                    if(button && button->get_window() == event.window)
                    {
                        button->draw();
                    }
                }
            }
        }

    if(SWITCHER && event.window == SWITCHER->get_menu_window())
        SWITCHER->raise();
    else if(RUN_MENU && event.window == RUN_MENU->get_menu_window())
        RUN_MENU->raise();
    else if(std::shared_ptr<EshyWMWindow> window = window_list_contains_titlebar(event.window))
        window->update_titlebar();
}

void WindowManager::OnButtonPress(const XButtonEvent& event)
{
    //Pass the click event through
    XAllowEvents(DISPLAY, ReplayPointer, event.time);

    if(event.window != CONTEXT_MENU->get_menu_window())
    {
        CONTEXT_MENU->remove();
    }

    if(event.window == SWITCHER->get_menu_window())
    {
        SWITCHER->button_clicked(event.x, event.y);
        return;
    }
    else if(event.window == ROOT)
    {
        //Only make context menu if we click on the desktop, not within the bounds of any window
        bool b_make_context_menu = true;
        for(const std::shared_ptr<EshyWMWindow>& window : window_list)
        {
            const Rect geo = window->get_frame_geometry();
            if(event.x_root > geo.x && event.x_root < geo.x + geo.width && event.y_root > geo.y && event.y_root < geo.y + geo.height)
            {
                b_make_context_menu = false;
                break;
            }
        }

        if(b_make_context_menu)
        {
            CONTEXT_MENU->set_position(event.x_root, event.y_root);
            CONTEXT_MENU->show();
        }

        return;
    }

    if(std::shared_ptr<EshyWMWindow> fwindow = window_list_contains_frame(event.window))
    {
        focus_window(fwindow);
        manipulating_window_geometry = fwindow->get_frame_geometry();
    }
    else if (std::shared_ptr<EshyWMWindow> twindow = window_list_contains_titlebar(event.window))
    {
        if(event.button == Button1)
        {
            //Handle double click for maximize in the titlebar
            if(titlebar_double_click.window == event.window
            && event.time - titlebar_double_click.first_click_time < EshyWMConfig::double_click_time)
            {
                twindow->maximize_window(twindow->get_window_state() != WS_MAXIMIZED);
                titlebar_double_click = {event.window, 0, event.time};
            }
            else titlebar_double_click = {event.window, event.time, 0};
        }
        else if (event.button == Button3)
        {
            //Minimize the window
            twindow->minimize_window(true);
        }
    }
    else return;

    //Save cursor position on click
    click_cursor_position = {event.x_root, event.y_root};
    XRaiseWindow(DISPLAY, event.window);
}

void WindowManager::OnButtonRelease(const XButtonEvent& event)
{
    //Pass the click event through
    XAllowEvents(DISPLAY, ReplayPointer, event.time);

    if(currently_hovered_button)
        currently_hovered_button->click();
}

void WindowManager::OnMotionNotify(const XMotionEvent& event)
{
    if(event.window == ROOT)
    {
        if(!b_at_bottom_right_corner && event.x_root >= EshyWM::monitors[0]->width - 10 && event.y_root >= EshyWM::monitors[0]->height - 10)
        {
            b_at_bottom_right_corner = true;

            if(view_desktop_window_list.empty())
                for(std::shared_ptr<EshyWMWindow> window : window_list)
                {
                    if(window->get_window_state() != WS_MINIMIZED)
                    {
                        view_desktop_window_list.emplace(window, window->get_window_state());
                        window->minimize_window(true, WSCC_MANUAL);
                    }
                }
            else
            {
                for(auto [window, window_state] : view_desktop_window_list)
                {
                    //We need to unminimize windows and restore their previous states
                    window->minimize_window(false, WSCC_MANUAL);

                    switch(window_state)
                    {
                    case WS_MAXIMIZED:
                        window->maximize_window(true, WSCC_MANUAL);
                        break;
                    case WS_FULLSCREEN:
                        window->fullscreen_window(true, WSCC_MANUAL);
                        break;
                    }
                }
                
                view_desktop_window_list.clear();
            }
        }
        else if (b_at_bottom_right_corner && (event.x_root < EshyWM::monitors[0]->width - 10 && event.y_root < EshyWM::monitors[0]->height - 10))
            b_at_bottom_right_corner = false;
        
        return;
    }

    const Pos delta = {event.x_root - click_cursor_position.x, event.y_root - click_cursor_position.y};

    std::shared_ptr<EshyWMWindow> window = window_list_contains_frame(event.window);
    if(window && event.state & Mod4Mask)
    {
        if(event.state & Button1Mask)
            window->move_window_absolute(manipulating_window_geometry.x + delta.x, manipulating_window_geometry.y + delta.y);
        else if(event.state & Button3Mask)
            window->resize_window_absolute(std::max((int)manipulating_window_geometry.width + delta.x, 10), std::max((int)manipulating_window_geometry.height + delta.y, 10));
    }
    else if(window = window_list_contains_titlebar(event.window))
    {
        if(window && event.time - titlebar_double_click.last_double_click_time > 10)
            window->move_window_absolute(manipulating_window_geometry.x + delta.x, manipulating_window_geometry.y + delta.y);
    }
}

void WindowManager::OnKeyPress(const XKeyEvent& event)
{
    std::shared_ptr<EshyWMWindow> window = window_list_contains_frame(event.window);

    if(event.window == ROOT)
    {
        CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod1Mask, XK_Tab)
        {
            SWITCHER->show();
            SWITCHER->next_option();
        }
        ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod4Mask, XK_R)
	    system("rofi -show drun");
        ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod4Mask, XK_e | XK_E)
        EshyWM::b_terminate = true;

        //If none of those, then check the keybindings
        for(const EshyWMConfig::KeyBinding& key_binding : EshyWMConfig::key_bindings)
        {
            if(event.keycode == XKeysymToKeycode(DISPLAY, XK_F6))
            {
                system(key_binding.command.c_str());
                break;
            }
        }
    }
    else if(event.state & Mod4Mask && window)
    {
        b_manipulating_with_keys = true;
        manipulating_window_geometry = window->get_frame_geometry();

        CHECK_KEYSYM_PRESSED(event, XK_C)
        EshyWMWindow::close_window(window);
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_d | XK_D)
        EshyWMWindow::toggle_maximize(window);
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_f | XK_F)
        EshyWMWindow::toggle_fullscreen(window);
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_a | XK_A)
        EshyWMWindow::toggle_minimize(window);
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_Left)
        {
            if(event.state & ShiftMask && event.state & ControlMask)
                window->resize_window_absolute(std::max((int)manipulating_window_geometry.width - EshyWMConfig::window_width_resize_step, 10), manipulating_window_geometry.height);
            else if(event.state & ShiftMask)
                window->attempt_shift_monitor(WS_ANCHORED_LEFT);
            else if(event.state & ControlMask)
                window->move_window_absolute(manipulating_window_geometry.x - EshyWMConfig::window_x_movement_step, manipulating_window_geometry.y);
            else
                window->anchor_window(WS_ANCHORED_LEFT);
        }
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_Up)
        {
            if(event.state & ShiftMask && event.state & ControlMask)
                window->resize_window_absolute(manipulating_window_geometry.width, std::max((int)manipulating_window_geometry.height - EshyWMConfig::window_height_resize_step, 10));
            else if(event.state & ShiftMask)
                window->attempt_shift_monitor(WS_ANCHORED_UP);
            else if(event.state & ControlMask)
                window->move_window_absolute(manipulating_window_geometry.x, manipulating_window_geometry.y - EshyWMConfig::window_y_movement_step);
            else
                window->anchor_window(WS_ANCHORED_UP);
        }
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_Right)
        {
            if(event.state & ShiftMask && event.state & ControlMask)
                window->resize_window_absolute(std::max((int)manipulating_window_geometry.width + EshyWMConfig::window_width_resize_step, 10), manipulating_window_geometry.height);
            else if(event.state & ShiftMask)
                window->attempt_shift_monitor(WS_ANCHORED_RIGHT);
            else if(event.state & ControlMask)
                window->move_window_absolute(manipulating_window_geometry.x + EshyWMConfig::window_x_movement_step, manipulating_window_geometry.y);
            else
                window->anchor_window(WS_ANCHORED_RIGHT);
        }
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_Down)
        {
            if(event.state & ShiftMask && event.state & ControlMask)
                window->resize_window_absolute(manipulating_window_geometry.width, std::max((int)manipulating_window_geometry.height + EshyWMConfig::window_height_resize_step, 10));
            else if(event.state & ShiftMask)
                window->attempt_shift_monitor(WS_ANCHORED_DOWN);
            else if(event.state & ControlMask)
                window->move_window_absolute(manipulating_window_geometry.x, manipulating_window_geometry.y + EshyWMConfig::window_y_movement_step);
            else
                window->anchor_window(WS_ANCHORED_DOWN);
        }
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_o | XK_O)
        increment_window_transparency(event.window, EshyWMConfig::window_opacity_step);
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_i | XK_I)
        decrement_window_transparency(event.window, EshyWMConfig::window_opacity_step);
    }

    XAllowEvents(DISPLAY, ReplayKeyboard, event.time);        
}

void WindowManager::OnKeyRelease(const XKeyEvent& event)
{
    if(event.window == ROOT && SWITCHER && SWITCHER->get_menu_active())
    {
        CHECK_KEYSYM_PRESSED(event, XK_Alt_L)
        SWITCHER->confirm_choice();
    }

    b_manipulating_with_keys = false;
}

void WindowManager::OnEnterNotify(const XCrossingEvent& event)
{
    if(std::shared_ptr<EshyWMWindow> window = window_list_contains_frame(event.window))
    {
        if(!b_manipulating_with_keys)
            focus_window(window);
    }
    else
        handle_button_hovered(event.window, true, event.mode);
}

void WindowManager::OnClientMessage(const XClientMessageEvent& event)
{
    static Atom ATOM_FULLSCREEN = XInternAtom(DISPLAY, "_NET_WM_STATE_FULLSCREEN", False);
    static Atom ATOM_STATE = XInternAtom(DISPLAY, "_NET_WM_STATE", False);

    if(std::shared_ptr<EshyWMWindow> window = window_list_contains_window(event.window))
        if(event.message_type == ATOM_STATE && (event.data.l[1] == ATOM_FULLSCREEN || event.data.l[2] == ATOM_FULLSCREEN))
            window->fullscreen_window(event.data.l[0], event.data.l[0] ? WSCC_STORE_STATE : WSCC_MANUAL);
}


std::shared_ptr<EshyWMWindow> WindowManager::window_list_contains_frame(Window frame)
{
    std::shared_ptr<EshyWMWindow> parent_window = nullptr;
    for(std::shared_ptr<EshyWMWindow> window : window_list)
    {
        if(window->get_frame() == frame)
        {
            parent_window = window;
            break;
        }
    }
    
    return parent_window;
}

std::shared_ptr<EshyWMWindow> WindowManager::window_list_contains_titlebar(Window titlebar)
{
    std::shared_ptr<EshyWMWindow> parent_window = nullptr;
    for(std::shared_ptr<EshyWMWindow> window : window_list)
    {
        if(window->get_titlebar() == titlebar)
        {
            parent_window = window;
            break;
        }
    }

    return parent_window;
}

std::shared_ptr<EshyWMWindow> WindowManager::window_list_contains_window(Window internal_window)
{
    std::shared_ptr<EshyWMWindow> parent_window = nullptr;
    for(std::shared_ptr<EshyWMWindow> window : window_list)
    {
        if(window->get_window() == internal_window)
        {
            parent_window = window;
            break;
        }
    }

    return parent_window;
}

void WindowManager::remove_from_window_list(std::shared_ptr<EshyWMWindow> window)
{
    window_list.erase(find(window_list.begin(), window_list.end(), window));
}

