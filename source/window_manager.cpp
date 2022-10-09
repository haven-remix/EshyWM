#include "window_manager.h"
#include "eshywm.h"
#include "config.h"

extern "C" {
#include <X11/Xutil.h>
}

#include <glog/logging.h>

#include <cstring>
#include <iostream>
#include <algorithm>

bool WindowManager::b_wm_detected;
std::mutex WindowManager::mutex_wm_detected;
Display* WindowManager::display;


std::shared_ptr<WindowManager> WindowManager::Create(const std::string& display_str)
{
    display = XOpenDisplay(display_str.empty() ? nullptr : display_str.c_str());

    if(display == nullptr)
    {
        LOG(ERROR) << "Failed to open X display " << XDisplayName(nullptr);
        return nullptr;
    }

    return std::make_shared<WindowManager>();
}

WindowManager::WindowManager()
    : WM_PROTOCOLS(XInternAtom(display, "WM_PROTOCOLS", false))
    , WM_DELETE_WINDOW(XInternAtom(display, "WM_DELETE_WINDOW", false))
    , root(DefaultRootWindow(display))
    , manager_data(new window_manager_data())
{
    main_slot = new slot(true);
    current_slot = main_slot;
}

WindowManager::~WindowManager()
{
    XCloseDisplay(display);
}


void WindowManager::Run()
{
    std::lock_guard<std::mutex> lock(mutex_wm_detected);

    b_wm_detected = false;
    XSetErrorHandler(&WindowManager::OnWMDetected);
    XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask | StructureNotifyMask);
    XSync(display, false);

    if(b_wm_detected)
    {
        LOG(ERROR) << "Detected another window manager on display " << XDisplayString(display);
        return;
    }

    //Handle windows already existing when the window manager is started

    XSetErrorHandler(&WindowManager::OnXError);
    //Grab X server to prevent windows from changing under us
    XGrabServer(display);

    Window returned_root;
    Window returned_parent;
    Window* top_level_windows;

    unsigned int num_top_level_windows;
    XQueryTree(display, root, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows);
    CHECK_EQ(returned_root, root);

    //Register, frame, and setup grab events for each top level window
    for(unsigned int i = 0; i < num_top_level_windows; ++i)
    {
        initialize_window(register_window(top_level_windows[i], true), true);
    }

    //Free top-level window array
    XFree(top_level_windows);

    //Ungrab X server
    XUngrabServer(display);

    //Main event loop
    for (;;)
    {
        main_loop();
    }
}

void WindowManager::main_loop()
{
    //Get next event
    XEvent event;
    XNextEvent(display, &event);

    //Dispatch event
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
    case ButtonPress:
        OnButtonPress(event.xbutton);
        break;
    case MotionNotify:
        while (XCheckTypedWindowEvent(display, event.xmotion.window, MotionNotify, &event)) {}
        OnMotionNotify(event.xmotion);
        break;
    case KeyPress:
        OnKeyPress(event.xkey);
        break;
    }

    for(auto &[titlebar, window] : window_titlebar_list)
    {
        window->draw_titlebar_buttons();
    }
}


void WindowManager::window_size_updated(EshyWMWindow* window)
{
    window->resize_window_absolute(window->get_preferred_size());
    for(auto &[frame, w] : window_frame_list)
    {
        if(w == window)
        {

        }
    }
}


int WindowManager::OnWMDetected(Display* display, XErrorEvent* event)
{
    CHECK_EQ(static_cast<int>(event->error_code), BadAccess);
    b_wm_detected = true;
    return 0;
}

int WindowManager::OnXError(Display* display, XErrorEvent* event)
{
    const int MAX_ERROR_TEXT_LEGTH = 1024;
    char error_text[MAX_ERROR_TEXT_LEGTH];

    XGetErrorText(display, event->error_code, error_text, sizeof(error_text));

    LOG(ERROR) << "Received X error:\n"
               << "    Request: " << int(event->request_code)
               << " - " << XRequestCodeToString(event->request_code) << "\n"
               << "    Error code: " << int(event->error_code)
               << " - " << error_text << "\n"
               << "    Resource ID: " << event->resourceid;

    return 0;
}


void WindowManager::OnConfigureNotify(const XConfigureEvent& event)
{
    if(event.window == root)
    {
        //Notify screen resolution changed
        EshyWM::Get().on_screen_resolution_changed();
    }
}

void WindowManager::OnUnmapNotify(const XUnmapEvent& event)
{
    //If we manage window then unframe. Need to check bcause we will receive an UnmapNotify event for a fram window we just destroyed
    if(!window_frame_list.count(event.window))
    {
        LOG(INFO) << "Ignored UnmapNotify for non-client window " << event.window;
        return;
    }

    //Ignore event if it is triggered by reparenting a window that was mapped before the window manager started
    if(event.event == root)
    {
        LOG(INFO) << "Ignored UnmapNotify for reparented pre-existing window " << event.window;
        return;
    }

    window_frame_list[event.window]->unframe_window();
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

    if(window_frame_list.count(event.window))
    {
        const Window frame = event.window;
        XConfigureWindow(display, frame, event.value_mask, &changes);
        LOG(INFO) << "Resize [" << frame << "] to " << size<int>(event.width, event.height);
    }

    //Grant request
    XConfigureWindow(display, event.window, event.value_mask, &changes);
    LOG(INFO) << "Resize " << event.window << " to " << size<int>(event.width, event.height);
}

void WindowManager::OnMapRequest(const XMapRequestEvent& event)
{
    initialize_window(register_window(event.window, false), false);
    //Map window
    XMapWindow(display, event.window);
}

void WindowManager::OnButtonPress(const XButtonEvent& event)
{
    //Check in case the last press closed the window
    if(window_frame_list.count(event.window) || window_titlebar_list.count(event.window))
    {
        //Save initial cursor position
        drag_start_position = Vector2D<int>(event.x_root, event.y_root);

        EshyWMWindow* window = window_titlebar_list.count(event.window) ? window_titlebar_list[event.window] : window_frame_list[event.window];
        window->recalculate_all_window_size_and_location();

        XRaiseWindow(display, event.window);
        XSetInputFocus(display, event.window, RevertToPointerRoot, event.time);
        //Pass the click event through
        XAllowEvents(display, ReplayPointer, event.time);

        check_titlebar_button_pressed(event.window, event.x, event.y);
    }
}

void WindowManager::OnMotionNotify(const XMotionEvent& event)
{
    const Vector2D<int> delta = Vector2D<int>(event.x_root, event.y_root) - drag_start_position;

    if(window_titlebar_list.count(event.window))
    {
        if(event.state & Button1Mask)
        {
            const window_size_location_data data = window_titlebar_list[event.window]->get_frame_size_and_location_data();
            const Vector2D<int> dest_frame_position = Vector2D<int>(data.x, data.y) + delta;
            XMoveWindow(display, window_titlebar_list[event.window]->get_frame(), dest_frame_position.x, dest_frame_position.y);
            window_titlebar_list[event.window]->draw_titlebar_buttons();
        }
        else if (event.state & Button3Mask)
        {
            window_titlebar_list[event.window]->resize_window(delta);
        }
    }
    else
    {
        if(event.state & Button1Mask)
        {
            const window_size_location_data data = window_frame_list[event.window]->get_frame_size_and_location_data();
            const Vector2D<int> dest_frame_position = Vector2D<int>(data.x, data.y) + delta;
            XMoveWindow(display, event.window, dest_frame_position.x, dest_frame_position.y);
            window_frame_list[event.window]->draw_titlebar_buttons();
        }
        else if(event.state & Button3Mask)
        {
            window_frame_list[event.window]->resize_window(delta);
        }
    }    
}

void WindowManager::OnKeyPress(const XKeyEvent& event)
{
    if ((event.state & Mod1Mask) && (event.keycode == XKeysymToKeycode(display, XK_C)))
    {
        close_window(event.window);
    }
    else if ((event.state & Mod1Mask) && (event.keycode == XKeysymToKeycode(display, XK_Tab)))
    {
        //Find next window
        auto i = window_frame_list.find(event.window);
        CHECK(i != window_frame_list.end());
        i++;
        if(i == window_frame_list.end())
        {
            i = window_frame_list.begin();
        }

        //Raise and set focus
        XRaiseWindow(display, i->first);
        XSetInputFocus(display, window_frame_list[i->first]->get_window(), RevertToPointerRoot, CurrentTime);
    }
    else if((event.state & Mod1Mask) && (event.keycode == XKeysymToKeycode(display, XK_Left)))
    {
        window_frame_list[event.window]->resize_window_horizontal_left_arrow();
    }
    else if((event.state & Mod1Mask) && (event.keycode == XKeysymToKeycode(display, XK_Right)))
    {
        window_frame_list[event.window]->resize_window_horizontal_right_arrow();
    }
    else if((event.state & Mod1Mask) && (event.keycode == XKeysymToKeycode(display, XK_Up)))
    {
        window_frame_list[event.window]->resize_window_vertical_up_arrow();
    }
    else if((event.state & Mod1Mask) && (event.keycode == XKeysymToKeycode(display, XK_Down)))
    {
        window_frame_list[event.window]->resize_window_vertical_down_arrow();
    }
}


EshyWMWindow* WindowManager::register_window(Window window, bool b_was_created_before_window_manager)
{
    //Retrieve attributes of window to frame
    XWindowAttributes x_window_attributes = {0};
    CHECK(XGetWindowAttributes(display, window, &x_window_attributes));

    //If window was created before window manager started, we should frame it only if it is visible and does not set override_redirect
    if(b_was_created_before_window_manager && (x_window_attributes.override_redirect || x_window_attributes.map_state != IsViewable))
    {
        return nullptr;
    }

    //Add so we can restore if we crash
    XAddToSaveSet(get_display(), window);

    EshyWMWindow* new_window = new EshyWMWindow(window);
    new_window->frame_window(b_was_created_before_window_manager);
    new_window->setup_grab_events(b_was_created_before_window_manager);
    window_frame_list.emplace(new_window->get_frame(), new_window);
    window_titlebar_list.emplace(new_window->get_titlebar(), new_window);

    return new_window;
}

void WindowManager::close_window(Window window)
{
    if(window_titlebar_list.count(window))
    {
        const EshyWMWindow* closed_window = window_titlebar_list[window];
        window_titlebar_list[window]->close_window();
        window_titlebar_list.erase(closed_window->get_titlebar());
        window_frame_list.erase(closed_window->get_frame());
        delete closed_window;
    }
    else if (window_frame_list.count(window))
    {
        const EshyWMWindow* closed_window = window_frame_list[window];
        window_frame_list[window]->close_window();
        window_frame_list.erase(closed_window->get_frame());
        window_titlebar_list.erase(closed_window->get_titlebar());
        delete closed_window;
    }
}

void WindowManager::initialize_window(EshyWMWindow* window, bool b_was_created_before_window_manager)
{
    if(!window)
    {
        return;
    }
    
    //Retrieve attributes of window to frame
    XWindowAttributes x_window_attributes = {0};
    XGetWindowAttributes(display, window->get_window(), &x_window_attributes);

    //If window was created before window manager started, we should frame it only if it is visible and does not set override_redirect
    if(b_was_created_before_window_manager && (x_window_attributes.override_redirect || x_window_attributes.map_state != IsViewable))
    {
        return;
    }

    current_slot->add_slot(window);
    current_slot->realign_content();
}

void WindowManager::check_titlebar_button_pressed(Window window, int cursor_x, int cursor_y)
{
    //This will check if the window is a frame (so we can't check the same area, but for a client window)
    if(window_titlebar_list.count(window))
    {
        const int button_pressed = window_titlebar_list[window]->is_cursor_on_titlebar_buttons(window, cursor_x, cursor_y);

        if(button_pressed == 1)
        {
            
        }
        else if(button_pressed == 2)
        {

        }
        else if(button_pressed == 3)
        {
            close_window(window);
        }
    }
}