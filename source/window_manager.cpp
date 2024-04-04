#include "window_manager.h"
#include "eshywm.h"
#include "window.h"
#include "config.h"
#include "taskbar.h"
#include "switcher.h"
#include "button.h"
#include "X11.h"

#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#include <cstring>
#include <algorithm>
#include <ranges>
#include <fstream>
#include <span>
#include <assert.h>

Display* WindowManager::display = nullptr;

void WindowManager::initialize()
{
    display = XOpenDisplay(nullptr);
    assert(display);

    XSetErrorHandler([](Display* display, XErrorEvent* event)
    {
        ensure((int)(event->error_code) == BadAccess)
        abort();
        return 0;
    });

    atoms.supported = XInternAtom(DISPLAY, "_NET_SUPPORTED", False);
    atoms.active_window = XInternAtom(DISPLAY, "_NET_ACTIVE_WINDOW", False);
    atoms.window_name = XInternAtom(DISPLAY, "WM_NAME", False);
    atoms.window_class = XInternAtom(DISPLAY, "WM_CLASS", False);
    atoms.wm_protocols = XInternAtom(DISPLAY, "WM_PROTOCOLS", False);
    atoms.wm_delete_window = XInternAtom(DISPLAY, "WM_DELETE_WINDOW", False);
    atoms.window_type = XInternAtom(DISPLAY, "_NET_WM_WINDOW_TYPE", False);
    atoms.window_type_dock = XInternAtom(DISPLAY, "_NET_WM_WINDOW_TYPE_DOCK", False);
    atoms.state = XInternAtom(DISPLAY, "_NET_WM_STATE", False);
    atoms.state_fullscreen = XInternAtom(DISPLAY, "_NET_WM_STATE_FULLSCREEN", False);

    Atom supported_atoms[2] = {atoms.active_window, atoms.window_name};
    XChangeProperty(DISPLAY, ROOT, atoms.supported, XA_ATOM, 32, PropModeReplace, (unsigned char*)supported_atoms, 2);

    XSelectInput(DISPLAY, ROOT, PointerMotionMask | SubstructureRedirectMask | StructureNotifyMask | SubstructureNotifyMask);
    XSync(DISPLAY, false);

    grab_keys();

    const int XC_left_ptr_code = 68;
    XDefineCursor(DISPLAY, ROOT, XCreateFontCursor(DISPLAY, XC_left_ptr_code));

    scan_outputs();

    //Create a deafult workspace for each output
    for (int i = 0; i < outputs.size(); ++i)
    {
        workspaces.emplace_back(new Workspace{ i, outputs[i] });
        outputs[i]->activate_workspace(workspaces.back());
    }

    //Make sure 9 workspaces exist
    for (int i = 0; i < 9 - outputs.size(); ++i)
    {
        workspaces.emplace_back(new Workspace{ i + (int)outputs.size(), outputs[i] });
    }
}

void WindowManager::handle_events()
{
    static XEvent event;

    while (XEventsQueued(DISPLAY, QueuedAfterFlush) != 0)
    {
        XNextEvent(DISPLAY, &event);
        LOG_EVENT_INFO(LS_Verbose, event);

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
            if (event.xcrossing.window != ROOT)
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

    const X11::WindowTree top_level_windows = X11::query_window_tree(ROOT);
    
    std::vector<Window> should_not_be_framed = { SWITCHER->get_menu_window() };
    for (Output* output : outputs)
    {
        if (output->top_dock)
            should_not_be_framed.push_back(output->top_dock->window);
        if (output->bottom_dock)
            should_not_be_framed.push_back(output->bottom_dock->window);
    }

    for(Window window : top_level_windows.windows)
    {
        if (std::ranges::contains(should_not_be_framed, window))
            continue;

        register_window(window, true);
        XMapWindow(DISPLAY, window);
    }

    for (unsigned int i = 0; i < num_top_level_windows; ++i)
    {
        if (std::ranges::contains(should_not_be_framed, top_level_windows[i]))
            continue;

        register_window(top_level_windows[i], true);
        XMapWindow(DISPLAY, top_level_windows[i]);
    }

    XFree(top_level_windows);
    XUngrabServer(DISPLAY);

    keep_raised = should_not_be_framed;
}

void WindowManager::grab_keys()
{
    for(EshyWMConfig::KeyBinding key_binding : EshyWMConfig::key_bindings)
    {
        grab_key(XKeysymToKeycode(DISPLAY, key_binding.key), Mod4Mask, ROOT);
    }

    grab_key(XKeysymToKeycode(DISPLAY, XK_e | XK_E), Mod4Mask, ROOT);

    //Workspace controls
    grab_key(XKeysymToKeycode(DISPLAY, XK_1), Mod4Mask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_2), Mod4Mask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_3), Mod4Mask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_4), Mod4Mask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_5), Mod4Mask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_6), Mod4Mask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_7), Mod4Mask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_8), Mod4Mask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_9), Mod4Mask, ROOT);

    /**WINDOW MANAGEMENT*/

    //Basic movement and resizing
    XGrabButton(DISPLAY, Button1, AnyModifier, ROOT, false, ButtonPressMask | ButtonReleaseMask, GrabModeSync, GrabModeAsync, None, None);
    XGrabButton(DISPLAY, Button3, AnyModifier, ROOT, false, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    grab_button(Button1, Mod4Mask, ROOT, ButtonMotionMask);
    grab_button(Button3, Mod4Mask, ROOT, ButtonMotionMask);

    //Basic functions
    grab_key(XKeysymToKeycode(DISPLAY, XK_c | XK_C), Mod4Mask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_d | XK_D), Mod4Mask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_f | XK_F), Mod4Mask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_a | XK_A), Mod4Mask, ROOT);

    //Anchors
    grab_key(XKeysymToKeycode(DISPLAY, XK_Left), Mod4Mask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_Up), Mod4Mask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_Right), Mod4Mask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_Down), Mod4Mask, ROOT);

    //Move monitors
    grab_key(XKeysymToKeycode(DISPLAY, XK_Left), Mod4Mask | ShiftMask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_Up), Mod4Mask | ShiftMask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_Right), Mod4Mask | ShiftMask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_Down), Mod4Mask | ShiftMask, ROOT);

    //Move window
    grab_key(XKeysymToKeycode(DISPLAY, XK_Left), Mod4Mask | ControlMask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_Up), Mod4Mask | ControlMask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_Right), Mod4Mask | ControlMask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_Down), Mod4Mask | ControlMask, ROOT);

    //Resize window
    grab_key(XKeysymToKeycode(DISPLAY, XK_Left), Mod4Mask | ShiftMask | ControlMask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_Up), Mod4Mask | ShiftMask | ControlMask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_Right), Mod4Mask | ShiftMask | ControlMask, ROOT);
    grab_key(XKeysymToKeycode(DISPLAY, XK_Down), Mod4Mask | ShiftMask | ControlMask, ROOT);
}

void WindowManager::ungrab_keys()
{
    XUngrabKey(DISPLAY, AnyKey, AnyModifier, ROOT);
}

void WindowManager::scan_outputs()
{
    int n_monitors;
    XRRMonitorInfo* found_monitors = XRRGetMonitors(DISPLAY, ROOT, false, &n_monitors);

    //If there are new monitors, then add them to outputs
    for (int i = 0; i < n_monitors; ++i)
    {
        const char* name = XGetAtomName(DISPLAY, found_monitors[i].name);

        if (std::ranges::count_if(outputs, [&name](const auto output) {return output->name.c_str() == name;}) > 0)
            continue;

        const Rect geometry = { found_monitors[i].x, found_monitors[i].y, (uint)found_monitors[i].width, (uint)found_monitors[i].height };
        outputs.emplace_back(new Output{ name, geometry, nullptr, nullptr, nullptr });
    }

    XRRFreeMonitors(found_monitors);
}


EshyWMWindow* WindowManager::register_window(Window window, bool b_was_created_before_window_manager)
{
    //Do not frame already framed windows
    if (std::ranges::count_if(window_list, [window](auto w) {return w->get_window() == window;}) > 0)
        return nullptr;

    //Retrieve attributes of window to frame
    XWindowAttributes x_window_attributes = { 0 };
    XGetWindowAttributes(DISPLAY, window, &x_window_attributes);

    //If window was created before window manager started, we should frame it only if it is visible and does not set override_redirect
    if (b_was_created_before_window_manager && (x_window_attributes.override_redirect || x_window_attributes.map_state != IsViewable))
        return nullptr;

    //Add so we can restore if we crash
    XAddToSaveSet(DISPLAY, window);

    //Center window, when creating we want to center it on the monitor the cursor is in
    const Pos cursor_position = get_cursor_position(DISPLAY, ROOT);

    Output* output = output_at_position(cursor_position.x, cursor_position.y);
    assert(output && output->active_workspace);

    x_window_attributes.width = std::min((int)(output->geometry.width * 0.9f), x_window_attributes.width);
    x_window_attributes.height = std::min((int)(output->geometry.height * 0.9f), x_window_attributes.height);
    
    //Set the x, y positions to be the center location
    x_window_attributes.x = std::clamp(center_x(output, x_window_attributes.width), output->geometry.x, center_x(output, 0));
    x_window_attributes.y = std::clamp(center_y(output, x_window_attributes.height), output->geometry.y, center_y(output, 0));

    //If window was previously maximized when it was closed, then maximize again. Otherwise center and clamp size
    const Atom ATOM_CLASS = XInternAtom(DISPLAY, "WM_CLASS", False);
    const XPropertyReturn class_property = get_xwindow_property(DISPLAY, window, ATOM_CLASS);

    const std::string window_name = std::string((const char*)class_property.property_value);
    const bool b_begin_maximized = EshyWMConfig::window_close_data.find(window_name) != EshyWMConfig::window_close_data.end() && EshyWMConfig::window_close_data[window_name] == "maximized";

    //Create window
    EshyWMWindow* new_window = new EshyWMWindow(window, output->active_workspace, false);
    new_window->frame_window(x_window_attributes);
    new_window->maximize_window(b_begin_maximized);
    window_list.push_back(new_window);

    EshyWM::window_created_notify(new_window);
    return new_window;
}

Dock* WindowManager::register_dock(Window window, bool b_was_created_before_window_manager)
{
    //Do not reregister docks
    auto check_registration = [window](auto output) {
        return (output->top_dock ? output->top_dock->window == window : false) || (output->bottom_dock ? output->bottom_dock->window == window : false);
    };
    if (std::ranges::count_if(outputs, check_registration) > 1)
        return nullptr;

    //Retrieve attributes of dock window
    XWindowAttributes x_window_attributes = { 0 };
    XGetWindowAttributes(DISPLAY, window, &x_window_attributes);

    //If dock was created before window manager started, we should register it only if it is visible and does not set override_redirect
    if (b_was_created_before_window_manager && (x_window_attributes.override_redirect || x_window_attributes.map_state != IsViewable))
        return nullptr;

    //Add so we can restore if we crash
    XAddToSaveSet(DISPLAY, window);

    const Rect geometry = { x_window_attributes.x, x_window_attributes.y, (uint)x_window_attributes.width, (uint)x_window_attributes.height };
    Output* output = output_most_occupied(geometry);
    assert(output);

    //Create dock
    Dock* new_dock = new Dock{ window, output, geometry, x_window_attributes.y == 0 ? DL_Top : DL_Bottom };
    output->add_dock(new_dock, new_dock->dock_location);
    return new_dock;
}

void WindowManager::focus_window(EshyWMWindow* window, bool b_raise)
{
    if (!window)
    {
        focused_window = nullptr;
        XSetInputFocus(DISPLAY, ROOT, RevertToNone, CurrentTime);
        return;
    }

    Window win = window->get_window();
    XChangeProperty(DISPLAY, ROOT, atoms.active_window, XA_WINDOW, 32, PropModeReplace, (unsigned char*)&win, 1);
    XSetInputFocus(DISPLAY, window->get_window(), RevertToNone, CurrentTime);

    focused_window = window;

    if(b_raise)
    {
        XRaiseWindow(DISPLAY, window->get_frame());

        //The window_list vector must be in the order that windows are displayed on screen
        //This is important because the focused window is not necessarly the top window
        auto it = std::ranges::find(window_list, window);
        std::rotate(window_list.begin(), it, window_list.end());

        //Switcher cares about the window stacking order, not focusing order
        SWITCHER->update_switcher_window_options();
    }
}


void WindowManager::handle_button_hovered(Window hovered_window, bool b_hovered, int mode)
{
    // if (currently_hovered_button)
    // {
    //     //Going from hovered to normal
    //     if (currently_hovered_button->get_button_state() == EButtonState::S_Hovered && !b_hovered && mode == NotifyNormal)
    //     {
    //         currently_hovered_button->set_button_state(EButtonState::S_Normal);
    //         currently_hovered_button = nullptr;
    //     }
    //     //Going from hovered to pressed
    //     else if (currently_hovered_button->get_button_state() == EButtonState::S_Hovered && !b_hovered && mode == NotifyGrab)
    //         currently_hovered_button->set_button_state(EButtonState::S_Pressed);
    //     //Going from pressed to hovered
    //     else if (currently_hovered_button->get_button_state() == EButtonState::S_Pressed && b_hovered && mode == NotifyUngrab)
    //         currently_hovered_button->set_button_state(EButtonState::S_Hovered);
    // }

    // //Going from normal to hovered (need extra logic because our currently_hovered_button will change here)
    // if (((!currently_hovered_button) || (currently_hovered_button && currently_hovered_button->get_button_state() == EButtonState::S_Normal)) && b_hovered)
    // {
    //     if(currently_hovered_button)
    //         currently_hovered_button->set_button_state(EButtonState::S_Normal);

    //     for(std::shared_ptr<s_monitor_info> monitor : EshyWM::monitors)
    //         for(std::shared_ptr<WindowButton>& button : monitor->taskbar->get_taskbar_buttons())
    //         {
    //             if(button->get_window() == hovered_window)
    //             {
    //                 currently_hovered_button = button;
    //                 break;
    //             }
    //         }

    //     if(!currently_hovered_button)
    //         for(EshyWMContainer* workspace : root_container->children)
    //         {
    //             for(EshyWMContainer* window_container : workspace->children)
    //             {
    //                 if(window_container->window->get_close_button() && window_container->window->get_close_button()->get_window() == hovered_window)
    //                 {
    //                     currently_hovered_button = window_container->window->get_close_button();
    //                     break;
    //                 }
    //             }
    //         }

    //     if(currently_hovered_button)
    //         currently_hovered_button->set_button_state(EButtonState::S_Hovered);
    // }
}


void WindowManager::OnDestroyNotify(const XDestroyWindowEvent& event)
{
    if (EshyWMWindow* window = contains_xwindow(event.window))
    {
        window->unframe_window();
        EshyWM::window_destroyed_notify(window);
        window_list.erase(std::ranges::find(window_list, window));

        if(focused_window == window)
        {
            focus_window(window_list.size() > 0 ? window_list[0] : nullptr, window_list.size() > 0  && focused_window == window_list[0] ? true : false);
        }

        //At this point the window and all pointers associated with it have been dealt with
        delete window;
    }
    else
    {
        Dock* found_dock = nullptr;
        for (Output* output : outputs)
        {
            found_dock = (output->top_dock && output->top_dock->window == event.window) ? output->top_dock
                : (output->bottom_dock && output->bottom_dock->window == event.window) ? output->bottom_dock
                : nullptr;

            if (found_dock)
            {
                found_dock->parent_output->remove_dock(found_dock);
                break;
            }
        }

        //At this point the dock and all pointers associated with it have been dealt with
        delete found_dock;
    }
}

void WindowManager::OnMapNotify(const XMapEvent& event)
{

}

void WindowManager::OnUnmapNotify(const XUnmapEvent& event)
{
    /* if(event.event == ROOT)
        return; */

    //@TODO:
    //I do not think this should be necessary because we should just use the minimize call
    //Commented for now, if I notice bugs due to this I will fix them later by calling minimize
    //Not deleting in case I actually need this at some point
    /* if (EshyWMWindow* window = contains_xwindow(event.window))
    {
        window->unframe_window();
    } */
}

void WindowManager::OnMapRequest(const XMapRequestEvent& event)
{
    //Check if this is a dock
    const XPropertyReturn window_type_property = get_xwindow_property(DISPLAY, event.window, atoms.window_type);
    const bool b_is_dock = window_type_property.status == Success && window_type_property.format == 32 && window_type_property.n_items > 0 &&
        std::ranges::contains(std::span((Atom*)window_type_property.property_value, window_type_property.n_items), atoms.window_type_dock);

    if(b_is_dock)
    {
        register_dock(event.window, false);
        XMapWindow(DISPLAY, event.window);
    }
    else if(EshyWMWindow* window = register_window(event.window, false))
    {
        XMapWindow(DISPLAY, event.window);
        focus_window(window, true);
    }
}

void WindowManager::OnPropertyNotify(const XPropertyEvent& event)
{
    //@TEMP: hashmap
    if (EshyWMWindow* window = contains_xwindow(event.window))
        window->update_titlebar();

    if (event.atom == atoms.window_type)
    {
        const XPropertyReturn window_type_property = get_xwindow_property(DISPLAY, event.window, atoms.window_type);
        const bool b_is_dock = window_type_property.status == Success && window_type_property.format == 32 && window_type_property.n_items > 0 &&
            std::ranges::contains(std::span((Atom*)window_type_property.property_value, window_type_property.n_items), atoms.window_type_dock);

        if(!b_is_dock)
            return;

        //Window type is dock. Unframe and dock the window.
        //@TEMP: hashmap
        if (EshyWMWindow* window = contains_xwindow(event.window))
            window->unframe_window();

        register_dock(event.window, false);
    }
}

void WindowManager::OnConfigureNotify(const XConfigureEvent& event)
{
    if (event.window == ROOT && event.display == DISPLAY)
    {
        scan_outputs();
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

    //@TEMP: hashmap
    if (EshyWMWindow* window = contains_xwindow(event.window))
    {
        changes.y -= EshyWMConfig::titlebar_height;
        XConfigureWindow(DISPLAY, window->get_frame(), event.value_mask, &changes);
    }
    else
        XConfigureWindow(DISPLAY, event.window, event.value_mask, &changes);
}

void WindowManager::OnVisibilityNotify(const XVisibilityEvent& event)
{
    if (std::ranges::contains(keep_raised, event.window))
    {
        XRaiseWindow(DISPLAY, event.window);
        return;
    }

    for(EshyWMWindow* window : window_list)
    {
        if(event.window != window->get_titlebar())
            continue;
        
        window->update_titlebar();
        break;
    }
}

void WindowManager::OnButtonPress(const XButtonEvent& event)
{
    //Pass the click event through
    XAllowEvents(DISPLAY, ReplayPointer, event.time);

    if(focused_window && is_within_rect(event.x_root, event.y_root, focused_window->get_frame_geometry()))
    {
        //LOGE("SKIPPING");
        goto skip_new_focus;
    }

    for(EshyWMWindow* window : window_list)
    {
        if(!is_within_rect(event.x_root, event.y_root, window->get_frame_geometry()))
            continue;
        
        focus_window(window, true);
        break;
    }

    if(!focused_window)
    {
        //LOGE("REMOVING FOCUS");
        focus_window(nullptr, false);
        return;
    }

skip_new_focus:

    //LOGE("Found focused");
    click_cursor_position = { event.x_root, event.y_root };
    manipulating_window_geometry = focused_window->get_frame_geometry();
    
    const bool b_in_titlebar = event.x_root >= focused_window->get_frame_geometry().x
        && event.x_root <= focused_window->get_frame_geometry().x + focused_window->get_frame_geometry().width
        && event.y_root >= focused_window->get_frame_geometry().y
        && event.y_root <= focused_window->get_frame_geometry().y + EshyWMConfig::titlebar_height;
    
    if(!b_in_titlebar)
        return;

    if (event.button == Button1)
    {
        //Handle double click for maximize in the titlebar
        if (titlebar_double_click.window == focused_window && event.time - titlebar_double_click.first_click_time < EshyWMConfig::double_click_time)
        {
            focused_window->toggle_maximize();
            titlebar_double_click = { focused_window, 0, event.time };
        }
        else titlebar_double_click = { focused_window, event.time, 0 };
    }
    else if (event.button == Button3)
    {
        focused_window->minimize_window(true);
    }
}

void WindowManager::OnButtonRelease(const XButtonEvent& event)
{
    //Pass the click event through
    XAllowEvents(DISPLAY, ReplayPointer, event.time);

    if (event.window == SWITCHER->get_menu_window())
    {
        SWITCHER->button_clicked(event.x, event.y);
        return;
    }

    if (currently_hovered_button)
        currently_hovered_button->click();
    
    b_manipulating_with_titlebar = false;
}

void WindowManager::OnMotionNotify(const XMotionEvent& event)
{
    if(!focused_window)
        return;

    const Pos delta = { event.x_root - click_cursor_position.x, event.y_root - click_cursor_position.y };

    if (event.state & Mod4Mask && event.state & Button1Mask)
    {
        focused_window->move_window_absolute(manipulating_window_geometry.x + delta.x, manipulating_window_geometry.y + delta.y, false);
    }
    else if (event.state & Mod4Mask && event.state & Button3Mask)
    {
        focused_window->resize_window_absolute(std::max((int)manipulating_window_geometry.width + delta.x, 10), std::max((int)manipulating_window_geometry.height + delta.y, 10), false);
    }
    else if (event.state & Button1Mask)
    {
        const bool b_in_titlebar = event.x_root >= focused_window->get_frame_geometry().x
            && event.x_root <= focused_window->get_frame_geometry().x + focused_window->get_frame_geometry().width
            && event.y_root >= focused_window->get_frame_geometry().y
            && event.y_root <= focused_window->get_frame_geometry().y + EshyWMConfig::titlebar_height;

        if(b_manipulating_with_titlebar || (b_in_titlebar && event.time - titlebar_double_click.last_double_click_time > 10))
        {
            b_manipulating_with_titlebar = true;
            focused_window->move_window_absolute(manipulating_window_geometry.x + delta.x, manipulating_window_geometry.y + delta.y, false);
        }
    }
}

void WindowManager::OnKeyPress(const XKeyEvent& event)
{
    if (event.window != ROOT)
    {
	    XAllowEvents(DISPLAY, ReplayKeyboard, event.time);
        return;
    }
    
    //Yes these are annoying, but they help readibility significantly
    #define CHECK_KEYSYM_PRESSED(event, key_sym)                    if(event.keycode == XKeysymToKeycode(DISPLAY, key_sym))
    #define ELSE_CHECK_KEYSYM_PRESSED(event, key_sym)               else if(event.keycode == XKeysymToKeycode(DISPLAY, key_sym))
    #define CHECK_KEYSYM_AND_MOD_PRESSED(event, mod, key_sym)       if(event.state & mod && event.keycode == XKeysymToKeycode(DISPLAY, key_sym))
    #define ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, mod, key_sym)  else if(event.state & mod && event.keycode == XKeysymToKeycode(DISPLAY, key_sym))

    #define CHECK_WINDOW_RESIZE_CONDITIONS                          if (event.state & ShiftMask && event.state & ControlMask)
    #define CHECK_WINDOW_SHIFT_MONITOR_CONDITIONS                   else if (event.state & ShiftMask)
    #define CHECK_WINDOW_MOVE_CONDITIONS                            else if (event.state & ControlMask)
    #define CHECK_WINDOW_ANCHOR_CONDITIONS                          else

    auto attempt_activate_workspace = [this](int num) {
        const Pos cursor_position = get_cursor_position(DISPLAY, ROOT);
        if (Output* output = output_at_position(cursor_position.x, cursor_position.y))
        {
            for (auto workspace : workspaces | std::views::filter([num](auto workspace) {return workspace->num == num;}))
                output->activate_workspace(workspace);
        }
    };

    CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod1Mask, XK_Tab)
    {
        SWITCHER->show();
        SWITCHER->next_option();
    }
    ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod4Mask, XK_e | XK_E)
    EshyWM::b_terminate = true;
    ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod4Mask, XK_1)
    attempt_activate_workspace(0);
    ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod4Mask, XK_2)
    attempt_activate_workspace(1);
    ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod4Mask, XK_3)
    attempt_activate_workspace(2);
    ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod4Mask, XK_4)
    attempt_activate_workspace(3);
    ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod4Mask, XK_5)
    attempt_activate_workspace(4);
    ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod4Mask, XK_6)
    attempt_activate_workspace(5);
    ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod4Mask, XK_7)
    attempt_activate_workspace(6);
    ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod4Mask, XK_8)
    attempt_activate_workspace(7);
    ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod4Mask, XK_9)
    attempt_activate_workspace(8);

    for(EshyWMConfig::KeyBinding key_binding : EshyWMConfig::key_bindings)
    {
        CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod4Mask, key_binding.key)
        system(key_binding.command.c_str());
    }

    if(!focused_window || !(event.state & Mod4Mask))
    {
	    XAllowEvents(DISPLAY, ReplayKeyboard, event.time);
        return;
    }

    b_manipulating_with_keys = true;
    manipulating_window_geometry = focused_window->get_frame_geometry();

    CHECK_KEYSYM_PRESSED(event, XK_C)
    focused_window->close_window();
    ELSE_CHECK_KEYSYM_PRESSED(event, XK_d | XK_D)
    focused_window->toggle_maximize();
    ELSE_CHECK_KEYSYM_PRESSED(event, XK_f | XK_F)
    focused_window->toggle_fullscreen();
    ELSE_CHECK_KEYSYM_PRESSED(event, XK_a | XK_A)
    focused_window->toggle_minimize();
    ELSE_CHECK_KEYSYM_PRESSED(event, XK_Left)
    {
        CHECK_WINDOW_RESIZE_CONDITIONS
        focused_window->resize_window_absolute(std::max((int)manipulating_window_geometry.width - EshyWMConfig::window_width_resize_step, 10), manipulating_window_geometry.height, false);
        CHECK_WINDOW_SHIFT_MONITOR_CONDITIONS
        focused_window->attempt_shift_monitor(WS_ANCHORED_LEFT);
        CHECK_WINDOW_MOVE_CONDITIONS
        focused_window->move_window_absolute(manipulating_window_geometry.x - EshyWMConfig::window_x_movement_step, manipulating_window_geometry.y, false);
        CHECK_WINDOW_ANCHOR_CONDITIONS
        focused_window->anchor_window(WS_ANCHORED_LEFT);
    }
    ELSE_CHECK_KEYSYM_PRESSED(event, XK_Up)
    {
        CHECK_WINDOW_RESIZE_CONDITIONS
        focused_window->resize_window_absolute(manipulating_window_geometry.width, std::max((int)manipulating_window_geometry.height - EshyWMConfig::window_height_resize_step, 10), false);
        CHECK_WINDOW_SHIFT_MONITOR_CONDITIONS
        focused_window->attempt_shift_monitor(WS_ANCHORED_UP);
        CHECK_WINDOW_MOVE_CONDITIONS
        focused_window->move_window_absolute(manipulating_window_geometry.x, manipulating_window_geometry.y - EshyWMConfig::window_y_movement_step, false);
        CHECK_WINDOW_ANCHOR_CONDITIONS
        focused_window->anchor_window(WS_ANCHORED_UP);
    }
    ELSE_CHECK_KEYSYM_PRESSED(event, XK_Right)
    {
        CHECK_WINDOW_RESIZE_CONDITIONS
        focused_window->resize_window_absolute(std::max((int)manipulating_window_geometry.width + EshyWMConfig::window_width_resize_step, 10), manipulating_window_geometry.height, false);
        CHECK_WINDOW_SHIFT_MONITOR_CONDITIONS
        focused_window->attempt_shift_monitor(WS_ANCHORED_RIGHT);
        CHECK_WINDOW_MOVE_CONDITIONS
        focused_window->move_window_absolute(manipulating_window_geometry.x + EshyWMConfig::window_x_movement_step, manipulating_window_geometry.y, false);
        CHECK_WINDOW_ANCHOR_CONDITIONS
        focused_window->anchor_window(WS_ANCHORED_RIGHT);
    }
    ELSE_CHECK_KEYSYM_PRESSED(event, XK_Down)
    {
        CHECK_WINDOW_RESIZE_CONDITIONS
        focused_window->resize_window_absolute(manipulating_window_geometry.width, std::max((int)manipulating_window_geometry.height + EshyWMConfig::window_height_resize_step, 10), false);
        CHECK_WINDOW_SHIFT_MONITOR_CONDITIONS
        focused_window->attempt_shift_monitor(WS_ANCHORED_DOWN);
        CHECK_WINDOW_MOVE_CONDITIONS
        focused_window->move_window_absolute(manipulating_window_geometry.x, manipulating_window_geometry.y + EshyWMConfig::window_y_movement_step, false);
        CHECK_WINDOW_ANCHOR_CONDITIONS
        focused_window->anchor_window(WS_ANCHORED_DOWN);
    }

    #undef CHECK_KEYSYM_PRESSED
    #undef ELSE_CHECK_KEYSYM_PRESSED
    #undef CHECK_KEYSYM_AND_MOD_PRESSED
    #undef ELSE_CHECK_KEYSYM_AND_MOD_PRESSED
    #undef CHECK_WINDOW_RESIZE_CONDITIONS
    #undef CHECK_WINDOW_SHIFT_MONITOR_CONDITIONS
    #undef CHECK_WINDOW_MOVE_CONDITIONS
    #undef CHECK_WINDOW_ANCHOR_CONDITIONS

	XAllowEvents(DISPLAY, ReplayKeyboard, event.time);
}

void WindowManager::OnKeyRelease(const XKeyEvent& event)
{
    if(SWITCHER && SWITCHER->get_menu_active() && event.keycode == XKeysymToKeycode(DISPLAY, XK_Alt_L))
        SWITCHER->confirm_choice();

    b_manipulating_with_keys = false;
}   

void WindowManager::OnEnterNotify(const XCrossingEvent& event)
{
    auto it = std::ranges::find_if(window_list, [frame = event.window](auto w) {return w->get_frame() == frame;});
    if (it != window_list.end())
    {
        if (!b_manipulating_with_keys)
            focus_window(*it, false);
    }
    else
        handle_button_hovered(event.window, true, event.mode);
}

void WindowManager::OnClientMessage(const XClientMessageEvent& event)
{
    //@TEMP: hashmap
    EshyWMWindow* window = contains_xwindow(event.window);
    if (window && event.message_type == atoms.state && (event.data.l[1] == atoms.state_fullscreen || event.data.l[2] == atoms.state_fullscreen))
        window->fullscreen_window(event.data.l[0]);
}


EshyWMWindow* WindowManager::contains_xwindow(Window window)
{
    auto it = std::ranges::find_if(window_list, [window](auto w) {return w->get_window() == window;});
    return it != window_list.end() ? *it : nullptr;
}