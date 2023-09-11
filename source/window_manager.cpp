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

#define CHECK_KEYSYM_PRESSED(event, key_sym)                    if(event.keycode == XKeysymToKeycode(DISPLAY, key_sym))
#define ELSE_CHECK_KEYSYM_PRESSED(event, key_sym)               else if(event.keycode == XKeysymToKeycode(DISPLAY, key_sym))
#define CHECK_KEYSYM_AND_MOD_PRESSED(event, mod, key_sym)       if(event.state & mod && event.keycode == XKeysymToKeycode(DISPLAY, key_sym))
#define ELSE_CHECK_KEYSYM_AND_MOD_PRESSED(event, mod, key_sym)  else if(event.state & mod && event.keycode == XKeysymToKeycode(DISPLAY, key_sym))

using namespace std::chrono_literals;

Display* WindowManager::display;
std::vector<std::shared_ptr<EshyWMWindow>> WindowManager::window_list;
std::vector<std::shared_ptr<s_monitor_info>> WindowManager::monitors;

static struct Vector2D
{
	int x;
	int y;
} click_cursor_position;

static struct double_click_data
{
    Window window;
    Time first_click_time;
    Time last_double_click_time;
} titlebar_double_click;

static bool b_manipulating_with_keys = false;

static std::shared_ptr<Button> currently_hovered_button;
static rect manipulating_window_geometry;

std::shared_ptr<EshyWMWindow> window_list_contains_frame(Window frame)
{
    std::shared_ptr<EshyWMWindow> parent_window = nullptr;
    for(std::shared_ptr<EshyWMWindow> window : WindowManager::window_list)
    {
        if(window->get_frame() == frame)
        {
            parent_window = window;
            break;
        }
    }
    
    return parent_window;
}

std::shared_ptr<EshyWMWindow> window_list_contains_titlebar(Window titlebar)
{
    std::shared_ptr<EshyWMWindow> parent_window = nullptr;
    for(std::shared_ptr<EshyWMWindow> window : WindowManager::window_list)
    {
        if(window->get_titlebar() == titlebar)
        {
            parent_window = window;
            break;
        }
    }

    return parent_window;
}

std::shared_ptr<EshyWMWindow> window_list_contains_window(Window internal_window)
{
    std::shared_ptr<EshyWMWindow> parent_window = nullptr;
    for(std::shared_ptr<EshyWMWindow> window : WindowManager::window_list)
    {
        if(window->get_window() == internal_window)
        {
            parent_window = window;
            break;
        }
    }

    return parent_window;
}

void remove_from_window_list(std::shared_ptr<EshyWMWindow> window)
{
    WindowManager::window_list.erase(find(WindowManager::window_list.begin(), WindowManager::window_list.end(), window));
}


static void update_monitor_info()
{
    int n_monitors;
    XRRMonitorInfo* found_monitors = XRRGetMonitors(DISPLAY, ROOT, false, &n_monitors);

    WindowManager::monitors.clear();

    for(int i = 0; i < n_monitors; i++)
    {
        std::shared_ptr<s_monitor_info> monitor_info = std::make_shared<s_monitor_info>((bool)found_monitors[i].primary, found_monitors[i].x, found_monitors[i].y, (uint)found_monitors[i].width, (uint)found_monitors[i].height);
        WindowManager::monitors.push_back(monitor_info);
    }

    XRRFreeMonitors(found_monitors);
}

static std::shared_ptr<EshyWMWindow> register_window(Window window, bool b_was_created_before_window_manager)
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

    std::shared_ptr<s_monitor_info> monitor;
    if(position_in_monitor(root_x, root_y, &monitor))
    {
        x_window_attributes.width = std::min((int)(monitor->width * 0.9f), x_window_attributes.width);
        x_window_attributes.height = std::min((int)((monitor->height - EshyWMConfig::taskbar_height) * 0.9f), x_window_attributes.height);
        x_window_attributes.x = std::clamp(CENTER_W(monitor, x_window_attributes.width), monitor->x, (int)(monitor->x + monitor->width) / 2);
        x_window_attributes.y = std::clamp(CENTER_H(monitor, x_window_attributes.height), monitor->y, (int)(monitor->y + monitor->height) / 2);
    }

    //Create window
    std::shared_ptr<EshyWMWindow> new_window = std::make_shared<EshyWMWindow>(window, false);
    new_window->frame_window(x_window_attributes);
    new_window->setup_grab_events();
    WindowManager::window_list.push_back(new_window);

    for(std::shared_ptr<s_monitor_info> monitor : WindowManager::monitors)
        monitor->taskbar->add_button(new_window, new_window->get_window_icon());

    SWITCHER->add_window_option(new_window, new_window->get_window_icon());
    return new_window;
}


static void handle_button_hovered(Window hovered_window, bool b_hovered, int mode)
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

        for(std::shared_ptr<s_monitor_info> monitor : WindowManager::monitors)
            for(std::shared_ptr<WindowButton>& button : monitor->taskbar->get_taskbar_buttons())
            {
                if(button->get_window() == hovered_window)
                {
                    currently_hovered_button = button;
                    break;
                }
            }

        if(!currently_hovered_button)
            for(std::shared_ptr<EshyWMWindow>& window : WindowManager::window_list)
            {
                if(window->get_minimize_button()->get_window() == hovered_window)
                    currently_hovered_button = window->get_minimize_button();
                else if(window->get_maximize_button()->get_window() == hovered_window)
                    currently_hovered_button = window->get_maximize_button();
                else if(window->get_close_button()->get_window() == hovered_window)
                    currently_hovered_button = window->get_close_button();

                if(currently_hovered_button)
                    break;
            }

        if(currently_hovered_button)
            currently_hovered_button->set_button_state(EButtonState::S_Hovered);
    }
}


static void OnDestroyNotify(const XDestroyWindowEvent& event)
{
    if(std::shared_ptr<EshyWMWindow> window = window_list_contains_frame(event.window))
    {
        for(std::shared_ptr<s_monitor_info> monitor : WindowManager::monitors)
            monitor->taskbar->remove_button(window);
        SWITCHER->remove_window_option(window);
        remove_from_window_list(window);
    }
}

static void OnMapNotify(const XMapEvent& event)
{
    
}

static void OnUnmapNotify(const XUnmapEvent& event)
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
}

static void OnMapRequest(const XMapRequestEvent& event)
{    
    register_window(event.window, false);
    XMapWindow(DISPLAY, event.window);
    XSetInputFocus(DISPLAY, event.window, RevertToPointerRoot, CurrentTime);
}

static void OnConfigureNotify(const XConfigureEvent& event)
{
    if(event.window == ROOT && event.display == DISPLAY)
    {
        update_monitor_info();

        //Notify screen resolution changed
        EshyWM::on_screen_resolution_changed(event.width, event.height);
    }
}

static void OnConfigureRequest(const XConfigureRequestEvent& event)
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

static void OnVisibilityNotify(const XVisibilityEvent& event)
{
    for(std::shared_ptr<s_monitor_info> monitor : WindowManager::monitors)
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

static void OnButtonPress(const XButtonEvent& event)
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
        for(const std::shared_ptr<EshyWMWindow>& window : WindowManager::window_list)
        {
            const rect geo = window->get_frame_geometry();
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
        EshyWMWindow::focus_window(fwindow);
        manipulating_window_geometry = fwindow->get_frame_geometry();
    }
    else if (std::shared_ptr<EshyWMWindow> twindow = window_list_contains_titlebar(event.window))
    {
        //Handle double click for maximize in the titlebar
        if(titlebar_double_click.window == event.window
        && event.time - titlebar_double_click.first_click_time < EshyWMConfig::double_click_time)
        {
            twindow->maximize_window(true);
            titlebar_double_click = {event.window, 0, event.time};
        }
        else titlebar_double_click = {event.window, event.time, 0};
    }
    else return;

    //Save cursor position on click
    click_cursor_position = {event.x_root, event.y_root};
    XRaiseWindow(DISPLAY, event.window);
}

static void OnButtonRelease(const XButtonEvent& event)
{
    //Pass the click event through
    XAllowEvents(DISPLAY, ReplayPointer, event.time);

    if(currently_hovered_button)
        currently_hovered_button->click();
}

static void OnMotionNotify(const XMotionEvent& event)
{
    const Vector2D delta = {event.x_root - click_cursor_position.x, event.y_root - click_cursor_position.y};

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

static void OnKeyPress(const XKeyEvent& event)
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
    }
    else if(event.state & Mod4Mask && window)
    {
        b_manipulating_with_keys = true;

        CHECK_KEYSYM_PRESSED(event, XK_C)
        EshyWMWindow::close_window(window);
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_d | XK_D)
        EshyWMWindow::toggle_maximize(window);
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_f | XK_F)
        EshyWMWindow::toggle_fullscreen(window);
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_a | XK_A)
        EshyWMWindow::toggle_minimize(window);
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_Left)
        window->anchor_window(WS_ANCHORED_LEFT);
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_Up)
        window->anchor_window(WS_ANCHORED_UP);
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_Right)
        window->anchor_window(WS_ANCHORED_RIGHT);
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_Down)
        window->anchor_window(WS_ANCHORED_DOWN);
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_o | XK_O)
        increment_window_transparency(event.window, EshyWMConfig::window_opacity_step);
        ELSE_CHECK_KEYSYM_PRESSED(event, XK_i | XK_I)
        decrement_window_transparency(event.window, EshyWMConfig::window_opacity_step);
    }

    XAllowEvents(DISPLAY, ReplayKeyboard, event.time);        
}

static void OnKeyRelease(const XKeyEvent& event)
{
    if(event.window == ROOT && SWITCHER && SWITCHER->get_menu_active())
    {
        CHECK_KEYSYM_PRESSED(event, XK_Alt_L)
        SWITCHER->confirm_choice();
    }

    b_manipulating_with_keys = false;
}

static void OnEnterNotify(const XCrossingEvent& event)
{
    if(std::shared_ptr<EshyWMWindow> window = window_list_contains_frame(event.window))
    {
        if(!b_manipulating_with_keys)
            window->focus_window(window);
    }
    else
        handle_button_hovered(event.window, true, event.mode);
}

static void OnClientMessage(const XClientMessageEvent& event)
{
    static Atom ATOM_FULLSCREEN = XInternAtom(DISPLAY, "_NET_WM_STATE_FULLSCREEN", False);
    static Atom ATOM_STATE = XInternAtom(DISPLAY, "_NET_WM_STATE", False);

    if(std::shared_ptr<EshyWMWindow> window = window_list_contains_window(event.window))
    {
        if(event.message_type == ATOM_STATE && (event.data.l[1] == ATOM_FULLSCREEN || event.data.l[2] == ATOM_FULLSCREEN))
        {
            EshyWMWindow::toggle_fullscreen(window);
        }
    }
}

namespace WindowManager
{
void initialize()
{
    display = XOpenDisplay(nullptr);
    ensure(display)

    static void* b_window_manager_detected = calloc(1, sizeof(bool));

    auto OnWMDetected = [](Display* display, XErrorEvent* event)
    {
        ensure(static_cast<int>(event->error_code) == BadAccess)
        *(bool*)b_window_manager_detected = true;
        return 0;
    };

    XSetErrorHandler(OnWMDetected);
    XSelectInput(DISPLAY, ROOT, SubstructureRedirectMask | StructureNotifyMask | SubstructureNotifyMask);
    XSync(DISPLAY, false);
    ensure(!(*(bool*)(b_window_manager_detected)))
    free(b_window_manager_detected);

    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_e | XK_E), AnyModifier, ROOT, false, GrabModeAsync, GrabModeSync);

    update_monitor_info();
    titlebar_double_click = {0, 0, 0};
}

void handle_events()
{
    static XEvent event;

    while(XEventsQueued(DISPLAY, QueuedAfterFlush) != 0)
    {
        XNextEvent(DISPLAY, &event);
        LOG_EVENT_INFO(LS_Verbose, event)
        
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
            handle_button_hovered(event.xcrossing.window, false, event.xcrossing.mode);
            break;
        case ClientMessage:
            OnClientMessage(event.xclient);
            break;
        };
    }
}

void handle_preexisting_windows()
{
    auto OnXError = [](Display* display, XErrorEvent* event)
    {
        const int MAX_ERROR_TEXT_LEGTH = 1024;
        char error_text[MAX_ERROR_TEXT_LEGTH];
        XGetErrorText(display, event->error_code, error_text, sizeof(error_text));
        return 0;
    };

    XSetErrorHandler(OnXError);
    XGrabServer(DISPLAY);

    Window returned_root;
    Window returned_parent;
    Window* top_level_windows;
    unsigned int num_top_level_windows;
    XQueryTree(DISPLAY, ROOT, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows);

    for(unsigned int i = 0; i < num_top_level_windows; ++i)
    {
        bool b_continue = false;
        for(std::shared_ptr<s_monitor_info> monitor : WindowManager::monitors)
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
};
