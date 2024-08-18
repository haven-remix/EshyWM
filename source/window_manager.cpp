#include "window_manager.h"
#include "eshywm.h"
#include "window.h"
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

void WindowManager::initialize()
{
    Display* display = XOpenDisplay(nullptr);
    assert(display);
    X11::set_display(display);

    XSetErrorHandler([](Display* display, XErrorEvent* event)
    {
        ensure((int)(event->error_code) == BadAccess)
        abort();
        return 0;
    });

    X11::atoms.cardinal = XInternAtom(display, "CARDINAL", False);
    X11::atoms.supported = XInternAtom(display, "_NET_SUPPORTED", False);
    X11::atoms.active_window = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    X11::atoms.window_name = XInternAtom(display, "WM_NAME", False);
    X11::atoms.window_class = XInternAtom(display, "WM_CLASS", False);
    X11::atoms.wm_protocols = XInternAtom(display, "WM_PROTOCOLS", False);
    X11::atoms.wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    X11::atoms.window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    X11::atoms.window_type_dock = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
    X11::atoms.window_icon = XInternAtom(display, "_NET_WM_ICON", False);
    X11::atoms.window_icon_name = XInternAtom(display, "WM_ICON_NAME", False);
    X11::atoms.state = XInternAtom(display, "_NET_WM_STATE", False);
    X11::atoms.state_fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

    const int num_supposed_atoms = 2;
    Atom supported_atoms[num_supposed_atoms] = {X11::atoms.active_window, X11::atoms.window_name};
    XChangeProperty(display, DefaultRootWindow(display), X11::atoms.supported, XA_ATOM, 32, PropModeReplace, (unsigned char*)supported_atoms, num_supposed_atoms);

    X11::set_input_masks(DefaultRootWindow(display), PointerMotionMask | SubstructureRedirectMask | StructureNotifyMask | SubstructureNotifyMask);
    XSync(display, false);

    grab_keys();
    scan_outputs();

    const int XC_left_ptr_code = 68;
    XDefineCursor(display, DefaultRootWindow(display), XCreateFontCursor(display, XC_left_ptr_code));

    //Create a deafult workspace for each output
    for (int i = 0; i < outputs.size(); ++i)
    {
        workspaces.emplace_back(std::make_shared<Workspace>( i, outputs[i] ));
        outputs[i]->activate_workspace(workspaces.back());
    }

    //Make sure 9 workspaces exist
    for (int i = 0; i < 9 - outputs.size(); ++i)
    {
        workspaces.emplace_back(std::make_shared<Workspace>( i + (int)outputs.size(), nullptr ));
    }
}

void WindowManager::handle_events()
{
    static XEvent event;

    while (XEventsQueued(X11::get_display(), QueuedAfterFlush) != 0)
    {
        XNextEvent(X11::get_display(), &event);
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
            while (XCheckTypedWindowEvent(X11::get_display(), event.xmotion.window, MotionNotify, &event)) {}
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
            if (event.xcrossing.window != X11::get_root_window())
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
        XGetErrorText(display, event->error_code, error_text, MAX_ERROR_TEXT_LEGTH);
        LOGE(error_text);
        return 0;
    });

    X11::grab_server();

    const X11::WindowTree top_level_windows = X11::query_window_tree(X11::get_root_window());
    assert(top_level_windows.status);
    for(Window window : top_level_windows.windows)
    {
        if (window == SWITCHER->get_menu_window())
            continue;

        register_window(window, true);
        X11::map_window(window);
    }

    X11::ungrab_server();
}

void WindowManager::grab_keys()
{
    const Window root = X11::get_root_window();

    for(EshyWMConfig::KeyBinding key_binding : EshyWMConfig::key_bindings)
    {
        X11::grab_key(key_binding.key, Mod4Mask, root);
    }

    X11::grab_key(XK_e | XK_E, Mod4Mask, root);
    X11::grab_key(XK_t | XK_T, Mod4Mask, root);
    X11::grab_key(XK_b | XK_B, Mod4Mask, root);

    //Workspace controls
    X11::grab_key(XK_1, Mod4Mask, root);
    X11::grab_key(XK_2, Mod4Mask, root);
    X11::grab_key(XK_3, Mod4Mask, root);
    X11::grab_key(XK_4, Mod4Mask, root);
    X11::grab_key(XK_5, Mod4Mask, root);
    X11::grab_key(XK_6, Mod4Mask, root);
    X11::grab_key(XK_7, Mod4Mask, root);
    X11::grab_key(XK_8, Mod4Mask, root);
    X11::grab_key(XK_9, Mod4Mask, root);

    /**WINDOW MANAGEMENT*/

    //Basic movement and resizing
    XGrabButton(X11::get_display(), Button1, AnyModifier, root, false, ButtonPressMask | ButtonReleaseMask, GrabModeSync, GrabModeAsync, None, None);
    XGrabButton(X11::get_display(), Button3, AnyModifier, root, false, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    X11::grab_button(Button1, Mod4Mask, root, ButtonMotionMask);
    X11::grab_button(Button3, Mod4Mask, root, ButtonMotionMask);

    //Basic functions
    X11::grab_key(XK_c | XK_C, Mod4Mask, root);
    X11::grab_key(XK_d | XK_D, Mod4Mask, root);
    X11::grab_key(XK_f | XK_F, Mod4Mask, root);
    X11::grab_key(XK_a | XK_A, Mod4Mask, root);

    //Anchors
    X11::grab_key(XK_Left, Mod4Mask, root);
    X11::grab_key(XK_Up, Mod4Mask, root);
    X11::grab_key(XK_Right, Mod4Mask, root);
    X11::grab_key(XK_Down, Mod4Mask, root);

    //Move monitors
    X11::grab_key(XK_Left, Mod4Mask | ShiftMask, root);
    X11::grab_key(XK_Up, Mod4Mask | ShiftMask, root);
    X11::grab_key(XK_Right, Mod4Mask | ShiftMask, root);
    X11::grab_key(XK_Down, Mod4Mask | ShiftMask, root);

    //Move window
    X11::grab_key(XK_Left, Mod4Mask | ControlMask, root);
    X11::grab_key(XK_Up, Mod4Mask | ControlMask, root);
    X11::grab_key(XK_Right, Mod4Mask | ControlMask, root);
    X11::grab_key(XK_Down, Mod4Mask | ControlMask, root);

    //Resize window
    X11::grab_key(XK_Left, Mod4Mask | ShiftMask | ControlMask, root);
    X11::grab_key(XK_Up, Mod4Mask | ShiftMask | ControlMask, root);
    X11::grab_key(XK_Right, Mod4Mask | ShiftMask | ControlMask, root);
    X11::grab_key(XK_Down, Mod4Mask | ShiftMask | ControlMask, root);
}

void WindowManager::ungrab_keys()
{
    X11::ungrab_key(AnyKey, AnyModifier, X11::get_root_window());
}

void WindowManager::scan_outputs()
{
    const X11::RRMonitorInfo found_monitors = X11::get_monitors();

    for(const XRRMonitorInfo monitor_info : found_monitors.monitors)
    {
        const std::string name = X11::get_atom_name(monitor_info.name);

        if (std::ranges::count_if(outputs, [&name](const auto output) {return output->name.c_str() == name;}) > 0)
            continue;

        const Rect geometry = {monitor_info.x, monitor_info.y, (uint)monitor_info.width, (uint)monitor_info.height};
        outputs.emplace_back(new Output{ name, geometry, nullptr, nullptr, nullptr });
    }
}


std::shared_ptr<EshyWMWindow> WindowManager::register_window(Window window, bool b_was_created_before_window_manager)
{
    if(contains_xwindow(window))
        return nullptr;

    const X11::WindowAttributes window_attributes = X11::get_window_attributes(window);

    //If window was created before window manager started, we should frame it only if it is visible and does not set override_redirect
    if (b_was_created_before_window_manager && (window_attributes.override_redirect || window_attributes.map_state != IsViewable))
        return nullptr;

    XAddToSaveSet(X11::get_display(), window);

    auto new_window = std::make_shared<EshyWMWindow>(window);
    new_window->initialize(window_attributes);
    new_window->frame_window();
    window_list.push_back(new_window);

    EshyWM::window_created_notify(new_window);
    return new_window;
}

std::shared_ptr<Dock> WindowManager::register_dock(Window window, bool b_was_created_before_window_manager)
{
    //Do not reregister docks
    for(auto output : outputs)
    {
        if (!(output->top_dock && output->top_dock->window == window) && !(output->bottom_dock && output->bottom_dock->window == window))
            continue;
        
        return nullptr;
    }

    //Retrieve attributes of dock window
    X11::WindowAttributes window_attributes = X11::get_window_attributes(window);

    //If dock was created before window manager started, we should register it only if it is visible and does not set override_redirect
    if (b_was_created_before_window_manager && (window_attributes.override_redirect || window_attributes.map_state != IsViewable))
        return nullptr;

    //Add so we can restore if we crash
    XAddToSaveSet(X11::get_display(), window);

    const Rect geometry = { window_attributes.x, window_attributes.y, (uint)window_attributes.width, (uint)window_attributes.height };
    auto output = output_most_occupied(geometry);
    assert(output);

    //Create dock
    auto new_dock = std::make_shared<Dock>(window, output, geometry, window_attributes.y == 0 ? DL_Top : DL_Bottom);
    output->add_dock(new_dock, new_dock->dock_location);
    return new_dock;
}

void WindowManager::focus_window(std::shared_ptr<EshyWMWindow> window, bool b_raise)
{
    if (!window)
    {
        focused_window = nullptr;
        X11::focus_window(X11::get_root_window());
        return;
    }

    Window win = window->get_window();
    X11::change_window_property(X11::get_root_window(), X11::atoms.active_window, XA_WINDOW, 32, (const unsigned char*)&win);
    X11::focus_window(window->get_window());

    focused_window = window;

    if(b_raise)
    {
        X11::raise_window(window->get_frame());

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
    if ((!currently_hovered_button || (currently_hovered_button && currently_hovered_button->get_button_state() == EButtonState::S_Normal)) && b_hovered)
    {
        if(currently_hovered_button)
            currently_hovered_button->set_button_state(EButtonState::S_Normal);

        if(!currently_hovered_button)
        {
            for(auto window : window_list)
            {
                if(window->get_close_button() && window->get_close_button()->get_window() == hovered_window)
                {
                    currently_hovered_button = window->get_close_button();
                    break;
                }
            }
        }

        if(currently_hovered_button)
            currently_hovered_button->set_button_state(EButtonState::S_Hovered);
    }
}


void WindowManager::OnDestroyNotify(const XDestroyWindowEvent& event)
{
    std::shared_ptr<Dock> found_dock = nullptr;
    for (auto output : outputs)
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
}

void WindowManager::OnMapNotify(const XMapEvent& event)
{

}

void WindowManager::OnUnmapNotify(const XUnmapEvent& event)
{
    /**
     * The reason this is here is because of stupid popups.
     * Reason this does not kill normal windows is because we only unmap the frame for those.
     * As a consequence, we need to keep it that way. Only unmap frames unless we want the thing dead.
     * Corallaraly, we do need to unmap the window to actually kill it.
     * 
     * Destroy notify is not run for popups so I cannot place this logic there.
    */
    if (auto window = contains_xwindow(event.window))
    {
        if(currently_hovered_button == window->get_close_button())
        {
            currently_hovered_button = nullptr;
        }
        
        window->unframe_window();
        EshyWM::window_destroyed_notify(window);
        window_list.erase(std::ranges::find(window_list, window));

        if(focused_window == window)
        {
            auto new_window = window_list.size() > 0 ? window_list[0] : nullptr;
            const bool b_raise_window = window_list.size() > 0 && focused_window == window_list[0];
            focus_window(new_window, b_raise_window);
        }
    }
}

void WindowManager::OnMapRequest(const XMapRequestEvent& event)
{
    //Check if this is a dock
    const X11::WindowProperty type_property = X11::get_window_property(event.window, X11::atoms.window_type);
    const bool b_is_dock = type_property.status == Success && type_property.format == 32 && type_property.n_items > 0 &&
        std::ranges::contains(std::span((Atom*)type_property.property_value, type_property.n_items), X11::atoms.window_type_dock);

    if(b_is_dock)
    {
        register_dock(event.window, false);
        X11::map_window(event.window);
    }
    else if(auto window = register_window(event.window, false))
    {
        X11::map_window(event.window);
        focus_window(window, true);
    }
}

void WindowManager::OnPropertyNotify(const XPropertyEvent& event)
{
    //@TEMP: hashmap
    if (auto window = contains_xwindow(event.window))
        window->update_titlebar();

    if (event.atom == X11::atoms.window_type)
    {
        const X11::WindowProperty type_property = X11::get_window_property(event.window, X11::atoms.window_type);
        const bool b_is_dock = type_property.status == Success && type_property.format == 32 && type_property.n_items > 0 &&
            std::ranges::contains(std::span((Atom*)type_property.property_value, type_property.n_items), X11::atoms.window_type_dock);

        if(!b_is_dock)
            return;

        //Window type is dock. Unframe and dock the window.
        if (auto window = contains_xwindow(event.window))
            window->unframe_window();

        register_dock(event.window, false);
    }
}

void WindowManager::OnConfigureNotify(const XConfigureEvent& event)
{
    if (event.window == X11::get_root_window() && event.display == X11::get_display())
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
    if (auto window = contains_xwindow(event.window))
    {
        changes.y -= EshyWMConfig::titlebar_height;
        XConfigureWindow(X11::get_display(), window->get_frame(), event.value_mask, &changes);
    }
    else
        XConfigureWindow(X11::get_display(), event.window, event.value_mask, &changes);
}

void WindowManager::OnVisibilityNotify(const XVisibilityEvent& event)
{
    for(auto window : window_list)
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
    X11::allow_events(ReplayPointer, event.time);

    if(currently_hovered_button)
    {
        currently_hovered_button->click();
        return;
    }

    if(window_list.size() > 0 && focused_window && is_within_rect(event.x_root, event.y_root, focused_window->get_frame_geometry()))
    {
        if(focused_window && focused_window != window_list[0])
        {
            focus_window(focused_window, true);
        }

        click_cursor_position = { event.x_root, event.y_root };
        manipulating_window_geometry = focused_window->get_frame_geometry();
        
        const bool b_in_titlebar = event.x_root >= focused_window->get_frame_geometry().x
            && event.x_root <= focused_window->get_frame_geometry().x + focused_window->get_frame_geometry().width
            && event.y_root >= focused_window->get_frame_geometry().y
            && event.y_root <= focused_window->get_frame_geometry().y + EshyWMConfig::titlebar_height;
        
        if(b_in_titlebar)
        {
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
    }
}

void WindowManager::OnButtonRelease(const XButtonEvent& event)
{
    //Pass the click event through
    X11::allow_events(ReplayPointer, event.time);

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
    if (event.window != X11::get_root_window())
    {
        X11::allow_events(ReplayKeyboard, event.time);
        return;
    }
    
    //Yes these are annoying, but they help readibility significantly
    #define CHECK_KEYSYM_PRESSED(event, key_sym)                    if(event.keycode == XKeysymToKeycode(X11::get_display(), key_sym))
    #define ELSE_CHECK_KEYSYM_PRESSED(event, key_sym)               else if(event.keycode == XKeysymToKeycode(X11::get_display(), key_sym))
    #define CHECK_KEYSYM_AND_MOD_PRESSED(event, mod, key_sym)       if(event.state & mod && event.keycode == XKeysymToKeycode(X11::get_display(), key_sym))
    #define ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, mod, key_sym)  else if(event.state & mod && event.keycode == XKeysymToKeycode(X11::get_display(), key_sym))

    #define CHECK_WINDOW_RESIZE_CONDITIONS                          if (event.state & ShiftMask && event.state & ControlMask)
    #define CHECK_WINDOW_SHIFT_MONITOR_CONDITIONS                   else if (event.state & ShiftMask)
    #define CHECK_WINDOW_MOVE_CONDITIONS                            else if (event.state & ControlMask)
    #define CHECK_WINDOW_ANCHOR_CONDITIONS                          else

    auto attempt_activate_workspace = [this](int num) {
        const Pos cursor_position = X11::get_cursor_position();
        if (auto output = output_at_position(cursor_position.x, cursor_position.y))
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
    ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod4Mask, XK_t | XK_T)
    {
        EshyWMConfig::titlebar = !EshyWMConfig::titlebar;
        for(auto window : window_list)
            window->set_show_titlebar(EshyWMConfig::titlebar);
    }
    ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, Mod4Mask, XK_b | XK_B)
    {
        b_show_window_borders = !b_show_window_borders;
        for(auto window : window_list)
            window->set_show_border(b_show_window_borders);
    }
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
        X11::allow_events(ReplayKeyboard, event.time);
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

    X11::allow_events(ReplayKeyboard, event.time);
}

void WindowManager::OnKeyRelease(const XKeyEvent& event)
{
    if(SWITCHER && SWITCHER->get_menu_active() && event.keycode == XKeysymToKeycode(X11::get_display(), XK_Alt_L))
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
    auto window = contains_xwindow(event.window);
    if (window && event.message_type == X11::atoms.state && (event.data.l[1] == X11::atoms.state_fullscreen || event.data.l[2] == X11::atoms.state_fullscreen))
        window->fullscreen_window(event.data.l[0]);
}


std::shared_ptr<EshyWMWindow> WindowManager::contains_xwindow(Window window)
{
    auto it = std::ranges::find_if(window_list, [window](auto w) {return w->get_window() == window;});
    return it != window_list.end() ? *it : nullptr;
}