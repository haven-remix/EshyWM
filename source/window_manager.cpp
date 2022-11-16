#include "window_manager.h"
#include "eshywm.h"
#include "window.h"
#include "config.h"
#include "context_menu.h"
#include "taskbar.h"
#include "switcher.h"
#include "run_menu.h"

#include <X11/Xutil.h>
#include <glog/logging.h>
#include <cstring>
#include <iostream>
#include <algorithm>

#define CHECK_KEYSYM_TO_KEYCODE(event, key_code)      if(event.keycode == XKeysymToKeycode(DISPLAY, key_code))
#define ELSE_CHECK_KEYSYM_TO_KEYCODE(event, key_code) else if(event.keycode == XKeysymToKeycode(DISPLAY, key_code))

Display* WindowManager::Internal::display;

Vector2D<int> WindowManager::Internal::click_cursor_position;
std::shared_ptr<class container> WindowManager::Internal::root_container;
window_manager_data* WindowManager::Internal::manager_data;

uint WindowManager::Internal::display_width;
uint WindowManager::Internal::display_height;

organized_container_map_t WindowManager::Internal::frame_list;
organized_container_map_t WindowManager::Internal::titlebar_list;

double_click_data WindowManager::Internal::titlebar_double_click;

static bool b_window_manager_detected;

bool is_key_down(Display* display, char* target_string)
{
    char keys_return[32] = {0};
    KeySym target_sym = XStringToKeysym(target_string);
    KeyCode target_code = XKeysymToKeycode(display, target_sym);

    int target_byte = target_code / 8;
    int target_bit = target_code % 8;
    int target_mask = 0x01 << target_bit;

    XQueryKeymap(display, keys_return);
    return keys_return[target_byte] & target_mask;
}

int OnWMDetected(Display* display, XErrorEvent* event)
{
    ensure(static_cast<int>(event->error_code) == BadAccess)
    b_window_manager_detected = true;
    return 0;
}

int OnXError(Display* display, XErrorEvent* event)
{
    const int MAX_ERROR_TEXT_LEGTH = 1024;
    char error_text[MAX_ERROR_TEXT_LEGTH];
    XGetErrorText(display, event->error_code, error_text, sizeof(error_text));
    return 0;
}


void OnUnmapNotify(const XUnmapEvent& event)
{
    /**If we manage window then unframe. Need to check
     * bcause we will receive an UnmapNotify event for
     * a frame window we just destroyed. Ignore event
     * if it is triggered by reparenting a window that
     * was mapped before the window manager started*/
    if(WindowManager::Internal::frame_list.count(event.window) && event.event != ROOT)
    {
        WindowManager::Internal::frame_list.at(event.window)->get_window()->unframe_window();
    }    
}

void OnConfigureNotify(const XConfigureEvent& event)
{
    if(event.window == ROOT && event.display == DISPLAY)
    {
        const uint previous_width = DISPLAY_WIDTH;
        const uint previous_height = DISPLAY_HEIGHT;
        WindowManager::Internal::display_width = event.width;
        WindowManager::Internal::display_height = event.height - EshyWMConfig::taskbar_height;
     
        //Notify screen resolution changed
        EshyWM::on_screen_resolution_changed(event.width, event.height);
        WindowManager::rescale_windows(previous_width, previous_height);
    }
}

void OnMapRequest(const XMapRequestEvent& event)
{
    WindowManager::register_window(event.window, false);
    //Map window
    XMapWindow(DISPLAY, event.window);
}

void OnConfigureRequest(const XConfigureRequestEvent& event)
{
    XWindowChanges changes;
    changes.x = event.x;
    changes.y = event.y;
    changes.width = event.width;
    changes.height = event.height;
    changes.border_width = event.border_width;
    changes.sibling = event.above;
    changes.stack_mode = event.detail;

    if(WindowManager::Internal::frame_list.count(event.window))
    {
        const Window frame = event.window;
        XConfigureWindow(DISPLAY, frame, event.value_mask, &changes);
    }

    //Grant request
    XConfigureWindow(DISPLAY, event.window, event.value_mask, &changes);
}

void OnVisibilityNotify(const XVisibilityEvent& event)
{
    if(TASKBAR && event.window == TASKBAR->get_menu_window())
    {
        TASKBAR->raise();
    }
    else if(SWITCHER && event.window == SWITCHER->get_menu_window())
    {
        SWITCHER->raise();
    }
    else if(RUN_MENU && event.window == RUN_MENU->get_menu_window())
    {
        RUN_MENU->raise();
    }
}

void OnButtonPress(const XButtonEvent& event)
{
    //Pass the click event through
    XAllowEvents(DISPLAY, ReplayPointer, event.time);

    if(event.window == ROOT)
    {
        Window returned_root;
        Window returned_parent;
        Window* top_level_windows;
        unsigned int num_top_level_windows;
        XQueryTree(DISPLAY, ROOT, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows);

        bool b_is_above_window = false;

        for(unsigned int i = 0; i < num_top_level_windows; i++)
        {
            if(top_level_windows[i] == ROOT)
            {
                continue;
            }

            XWindowAttributes attr;
            XGetWindowAttributes(DISPLAY, top_level_windows[i], &attr);

            if(event.x_root > attr.x && event.x_root < attr.x + attr.width && event.y_root > attr.y && event.y_root < attr.y + attr.height)
            {
                b_is_above_window = true;
                break;
            }
        }

        XFree(top_level_windows);

        if(!b_is_above_window)
        {
            CONTEXT_MENU->set_position(event.x, event.y);
            CONTEXT_MENU->show();
        }

        return;
    }

    if(event.window != CONTEXT_MENU->get_menu_window())
    {
        CONTEXT_MENU->remove();
    }

    if(event.window == TASKBAR->get_menu_window())
    {
        TASKBAR->check_taskbar_button_clicked(event.x, event.y);
        return;
    }

    if(WindowManager::Internal::frame_list.count(event.window))
    {
        WindowManager::Internal::frame_list.at(event.window)->get_window()->recalculate_all_window_size_and_location();
    }
    else if (WindowManager::Internal::titlebar_list.count(event.window))
    {
        WindowManager::Internal::titlebar_list.at(event.window)->get_window()->recalculate_all_window_size_and_location();
        WindowManager::check_titlebar_button_pressed(event.window, event.x, event.y);

        if(event.time - WindowManager::Internal::titlebar_double_click.first_click_time < EshyWMConfig::double_click_time && WindowManager::Internal::titlebar_double_click.first_click_window == event.window)
        WindowManager::Internal::titlebar_list.at(event.window)->get_window()->maximize_window();
        else WindowManager::Internal::titlebar_double_click = {event.window, event.time};
    }
    else return;

    //Save cursor position on click
    WindowManager::Internal::click_cursor_position = Vector2D<int>(event.x_root, event.y_root);
    XRaiseWindow(DISPLAY, event.window);
    XSetInputFocus(DISPLAY, event.window, RevertToPointerRoot, event.time);
}

void OnButtonRelease(const XButtonEvent& event)
{
    //Pass the click event through
    XAllowEvents(DISPLAY, ReplayPointer, event.time);

    if(event.state & Button1Mask || event.state & Button3Mask)
    {
        if(event.window == ROOT && event.state & Button3Mask)
        {
            CONTEXT_MENU->set_position(event.x, event.y);
            CONTEXT_MENU->show();
        }

        if(WindowManager::Internal::frame_list.count(event.window))
        {
            WindowManager::Internal::frame_list.at(event.window)->get_window()->motion_modify_ended();
        }
        else if (WindowManager::Internal::titlebar_list.count(event.window))
        {
            WindowManager::Internal::titlebar_list.at(event.window)->get_window()->motion_modify_ended();
        }
    }
}

void OnMotionNotify(const XMotionEvent& event)
{
    const Vector2D<int> delta = Vector2D<int>(event.x_root, event.y_root) - WindowManager::Internal::click_cursor_position;

    if(event.state & Mod4Mask && event.state & ShiftMask)
    {
        if(event.state & Button1Mask)
        {
           WindowManager::Internal::frame_list.at(event.window)->get_window()->move_window(delta);
           WindowManager::Internal::frame_list.at(event.window)->get_window()->draw();
        }
        else if(event.state & Button3Mask)
        {
           WindowManager::Internal::frame_list.at(event.window)->get_window()->resize_window(delta);
        }

        return;
    }

    if(WindowManager::Internal::titlebar_list.count(event.window))
    {
        WindowManager::Internal::titlebar_list.at(event.window)->get_window()->move_window(delta);
        WindowManager::Internal::titlebar_list.at(event.window)->get_window()->draw();
    }
}

void OnKeyPress(const XKeyEvent& event)
{
    XAllowEvents(DISPLAY, ReplayKeyboard, event.time);

    if(!(event.state & Mod4Mask))
    {
        return;
    }

    if(event.window == ROOT)
    {
        if(event.keycode == XKeysymToKeycode(DISPLAY, XK_Tab))
        {
            SWITCHER->show();
            SWITCHER->next_option();
        }
        else if (event.keycode == XKeysymToKeycode(DISPLAY, XK_R))
        {
            RUN_MENU->show();
        }
    }

    CHECK_KEYSYM_TO_KEYCODE(event, XK_C)
    WindowManager::Internal::frame_list.at(event.window)->get_window()->close_window();
    else if(event.state & ControlMask)
    {
        CHECK_KEYSYM_TO_KEYCODE(event, XK_Left)
        WindowManager::Internal::frame_list.at(event.window)->move_window_horizontal_left_arrow();
        ELSE_CHECK_KEYSYM_TO_KEYCODE(event, XK_Right)
        WindowManager::Internal::frame_list.at(event.window)->move_window_horizontal_right_arrow();
        ELSE_CHECK_KEYSYM_TO_KEYCODE(event, XK_Up)
        WindowManager::Internal::frame_list.at(event.window)->move_window_vertical_up_arrow();
        ELSE_CHECK_KEYSYM_TO_KEYCODE(event, XK_Down)
        WindowManager::Internal::frame_list.at(event.window)->move_window_vertical_down_arrow();
    }
    ELSE_CHECK_KEYSYM_TO_KEYCODE(event, XK_f | XK_F)
    WindowManager::Internal::frame_list.at(event.window)->get_window()->maximize_window();
    ELSE_CHECK_KEYSYM_TO_KEYCODE(event, XK_a | XK_A)
    WindowManager::Internal::frame_list.at(event.window)->get_window()->minimize_window();
}

void OnKeyRelease(const XKeyEvent& event)
{
    if(!is_key_down(DISPLAY, XKeysymToString(XK_Alt_L)))
    {
        SWITCHER->confirm_choice();
    }
}

namespace WindowManager
{
void initialize()
{
    Internal::display = XOpenDisplay(nullptr);
    ensure(Internal::display)

    Internal::display_width = DisplayWidth(Internal::display, DefaultScreen(Internal::display));
    Internal::display_height = DisplayHeight(Internal::display, DefaultScreen(Internal::display)) - EshyWMConfig::taskbar_height;

    Internal::manager_data = new window_manager_data();

    //std::lock_guard<std::mutex> lock(mutex_wm_detected);
    b_window_manager_detected = false;
    XSetErrorHandler(&OnWMDetected);
    XSelectInput(DISPLAY, ROOT, SubstructureRedirectMask | SubstructureNotifyMask | StructureNotifyMask);
    XSync(DISPLAY, false);
    ensure(!b_window_manager_detected)

    //Create root container
    Internal::root_container = std::make_shared<container>(EOrientation::O_Horizontal, EContainerType::CT_Root);
    Internal::root_container->set_size(DISPLAY_WIDTH, DISPLAY_HEIGHT);
}

void main_loop()
{
    //Make sure the lists are up to date
    Internal::frame_list = Internal::root_container->get_all_container_map_by_frame();
    Internal::titlebar_list = Internal::root_container->get_all_container_map_by_titlebar();

    XEvent event;
    XNextEvent(DISPLAY, &event);

    switch (event.type)
    {
    case UnmapNotify:
        OnUnmapNotify(event.xunmap);
        break;
    case ConfigureNotify:
        OnConfigureNotify(event.xconfigure);
        break;
    case MapRequest:
        OnMapRequest(event.xmaprequest);
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
    }

    for(auto &[titlebar, container] : Internal::titlebar_list)
    {
        container->get_window()->draw();
    }

    TASKBAR->draw();
    SWITCHER->draw();
    CONTEXT_MENU->draw();
}

void handle_preexisting_windows()
{
    XSetErrorHandler(&OnXError);
    XGrabServer(DISPLAY);

    Window returned_root;
    Window returned_parent;
    Window* top_level_windows;
    unsigned int num_top_level_windows;
    XQueryTree(DISPLAY, ROOT, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows);

    for(unsigned int i = 0; i < num_top_level_windows; ++i)
    {
        if(top_level_windows[i] != TASKBAR->get_menu_window()
        && top_level_windows[i] != SWITCHER->get_menu_window()
        && top_level_windows[i] != CONTEXT_MENU->get_menu_window())
        {
            register_window(top_level_windows[i], true);
        }
    }

    XFree(top_level_windows);
    XUngrabServer(DISPLAY);
}

std::shared_ptr<class EshyWMWindow> register_window(Window window, bool b_was_created_before_window_manager)
{
    //Retrieve attributes of window to frame
    XWindowAttributes x_window_attributes = {0};
    XGetWindowAttributes(DISPLAY, window, &x_window_attributes);

    //If window was created before window manager started, we should frame it only if it is visible and does not set override_redirect
    if(b_was_created_before_window_manager && (x_window_attributes.override_redirect || x_window_attributes.map_state != IsViewable))
    {
        return nullptr;
    }

    //Add so we can restore if we crash
    XAddToSaveSet(DISPLAY, window);

    std::shared_ptr<container> leaf_container = std::make_shared<container>(EOrientation::O_Horizontal, EContainerType::CT_Leaf);
    leaf_container->create_window(window);
    Internal::root_container->add_internal_container(leaf_container);
    TASKBAR->add_button(leaf_container->get_window());
    SWITCHER->add_window_option(leaf_container->get_window());
    return leaf_container->get_window();
}

void close_window(std::shared_ptr<class EshyWMWindow> closed_window)
{
    TASKBAR->remove_button(closed_window);
}

void rescale_windows(uint old_width, uint old_height)
{

}

void check_titlebar_button_pressed(Window window, int cursor_x, int cursor_y)
{
    //This will check if the window is a frame (so we can't check the same area, but for a client window)
    if(Internal::titlebar_list.count(window))
    {
        const int button_pressed = Internal::titlebar_list.at(window)->get_window()->is_cursor_on_titlebar_buttons(window, cursor_x, cursor_y);

        if(button_pressed == 1)
        Internal::titlebar_list.at(window)->get_window()->minimize_window();
        else if(button_pressed == 2)
        Internal::titlebar_list.at(window)->get_window()->maximize_window();
        else if(button_pressed == 3)
        Internal::titlebar_list.at(window)->get_window()->close_window();
    }
}
};