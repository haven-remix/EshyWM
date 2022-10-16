#include "window_manager.h"
#include "container.h"
#include "eshywm.h"
#include "window.h"
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
    display_width = DisplayWidth(display, DefaultScreen(display));
    display_height = DisplayHeight(display, DefaultScreen(display));
    root_container = std::make_shared<container>(EOrientation::O_Horizontal, EContainerType::CT_Root);
    root_container->set_size(display_width, display_height);
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
        register_window(top_level_windows[i], true);
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

    for(auto &[titlebar, container] : root_container->get_all_container_map_by_titlebar())
    {
        container->get_window()->draw_titlebar_buttons();
    }
}


void WindowManager::container_size_updated(std::weak_ptr<container> updated_container)
{
    
}

void WindowManager::container_moved(std::weak_ptr<container> moved_container)
{
    const ordered_container_map_t horizontal_list = root_container->get_all_container_map_by_horizontal_position();
    const ordered_container_map_t vertical_list = root_container->get_all_container_map_by_vertical_position();

    int from_previous = 0;
    for(auto &[p, c] : horizontal_list)
    {
        XMoveWindow(display, c->get_window()->get_frame(), from_previous, 0);
        from_previous += c->get_size_and_position().width;
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

    return 0;
}


void WindowManager::OnConfigureNotify(const XConfigureEvent& event)
{
    if(event.window == root)
    {
        display_width = DisplayWidth(display, DefaultScreen(display));
        display_height = DisplayHeight(display, DefaultScreen(display));
        //Notify screen resolution changed
        EshyWM::Get().on_screen_resolution_changed();
    }
}

void WindowManager::OnUnmapNotify(const XUnmapEvent& event)
{
    const organized_container_map_t frame_list = root_container->get_all_container_map_by_frame();

    //If we manage window then unframe. Need to check bcause we will receive an UnmapNotify event for a frame window we just destroyed
    if(!frame_list.count(event.window))
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

    frame_list.at(event.window)->get_window()->unframe_window();
}

void WindowManager::OnConfigureRequest(const XConfigureRequestEvent& event)
{
    const organized_container_map_t frame_list = root_container->get_all_container_map_by_frame();

    XWindowChanges changes;
    changes.x = event.x;
    changes.y = event.y;
    changes.width = event.width;
    changes.height = event.height;
    changes.border_width = event.border_width;
    changes.sibling = event.above;
    changes.stack_mode = event.detail;

    if(frame_list.count(event.window))
    {
        const Window frame = event.window;
        XConfigureWindow(display, frame, event.value_mask, &changes);
    }

    //Grant request
    XConfigureWindow(display, event.window, event.value_mask, &changes);
}

void WindowManager::OnMapRequest(const XMapRequestEvent& event)
{
    register_window(event.window, false);
    //Map window
    XMapWindow(display, event.window);
}

void WindowManager::OnButtonPress(const XButtonEvent& event)
{
    const organized_container_map_t frame_list = root_container->get_all_container_map_by_frame();
    const organized_container_map_t titlebar_list = root_container->get_all_container_map_by_titlebar();

    //Check in case the last press closed the window
    if(frame_list.count(event.window) || titlebar_list.count(event.window))
    {
        //Save initial cursor position
        drag_start_position = Vector2D<int>(event.x_root, event.y_root);

        std::shared_ptr<EshyWMWindow> window = titlebar_list.count(event.window) ? titlebar_list.at(event.window)->get_window() : frame_list.at(event.window)->get_window();
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
    const organized_container_map_t frame_list = root_container->get_all_container_map_by_frame();
    const organized_container_map_t titlebar_list = root_container->get_all_container_map_by_titlebar();

    const Vector2D<int> delta = Vector2D<int>(event.x_root, event.y_root) - drag_start_position;

    if(titlebar_list.count(event.window))
    {
        if(event.state & Button1Mask)
        {
            const rect data = titlebar_list.at(event.window)->get_window()->get_frame_size_and_location_data();
            const Vector2D<int> dest_frame_position = Vector2D<int>(data.x, data.y) + delta;
            XMoveWindow(display, titlebar_list.at(event.window)->get_window()->get_frame(), dest_frame_position.x, dest_frame_position.y);
            titlebar_list.at(event.window)->get_window()->draw_titlebar_buttons();
        }
        else if (event.state & Button3Mask)
        {
            titlebar_list.at(event.window)->get_window()->resize_window(delta);
        }
    }
    else
    {
        if(event.state & Button1Mask)
        {
            const rect data = frame_list.at(event.window)->get_window()->get_frame_size_and_location_data();
            const Vector2D<int> dest_frame_position = Vector2D<int>(data.x, data.y) + delta;
            XMoveWindow(display, event.window, dest_frame_position.x, dest_frame_position.y);
            frame_list.at(event.window)->get_window()->draw_titlebar_buttons();
        }
        else if(event.state & Button3Mask)
        {
            frame_list.at(event.window)->get_window()->resize_window(delta);
        }
    }    
}

void WindowManager::OnKeyPress(const XKeyEvent& event)
{
    if(!(event.state & Mod1Mask))
    {
        return;
    }

    const organized_container_map_t frame_list = root_container->get_all_container_map_by_frame();
    const organized_container_map_t titlebar_list = root_container->get_all_container_map_by_titlebar();

    if (event.keycode == XKeysymToKeycode(display, XK_C))
    {
        close_window(event.window);
    }
    else if (event.keycode == XKeysymToKeycode(display, XK_Tab))
    {
        //Find next window
        auto i = frame_list.find(event.window);
        CHECK(i != frame_list.end());
        i++;
        if(i == frame_list.end())
        {
            i = frame_list.begin();
        }

        //Raise and set focus
        XRaiseWindow(display, i->first);
        XSetInputFocus(display, frame_list.at(i->first)->get_window()->get_window(), RevertToPointerRoot, CurrentTime);
    }
    else if(event.state & ControlMask)
    {
        if (event.keycode == XKeysymToKeycode(display, XK_Left))
        {
            frame_list.at(event.window)->move_window_horizontal_left_arrow();
        }
        else if(event.keycode == XKeysymToKeycode(display, XK_Right))
        {
            frame_list.at(event.window)->move_window_horizontal_right_arrow();
        }
        else if(event.keycode == XKeysymToKeycode(display, XK_Up))
        {
            frame_list.at(event.window)->move_window_vertical_up_arrow();
        }
        else if(event.keycode == XKeysymToKeycode(display, XK_Down))
        {
            frame_list.at(event.window)->move_window_vertical_down_arrow();
        }
    }
    else if(event.keycode == XKeysymToKeycode(display, XK_Left))
    {
        frame_list.at(event.window)->get_window()->resize_window_horizontal_left_arrow();
    }
    else if(event.keycode == XKeysymToKeycode(display, XK_Right))
    {
        frame_list.at(event.window)->get_window()->resize_window_horizontal_right_arrow();
    }
    else if(event.keycode == XKeysymToKeycode(display, XK_Up))
    {
        frame_list.at(event.window)->get_window()->resize_window_vertical_up_arrow();
    }
    else if(event.keycode == XKeysymToKeycode(display, XK_Down))
    {
        frame_list.at(event.window)->get_window()->resize_window_vertical_down_arrow();
    }
}


std::shared_ptr<EshyWMWindow> WindowManager::register_window(Window window, bool b_was_created_before_window_manager)
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

    std::shared_ptr<container> leaf_container = std::make_shared<container>(EOrientation::O_Horizontal, EContainerType::CT_Leaf);
    leaf_container->create_window(window);
    root_container->add_internal_container(leaf_container);
    return leaf_container->get_window();
}

void WindowManager::close_window(Window window)
{
    const organized_container_map_t frame_list = root_container->get_all_container_map_by_frame();
    const organized_container_map_t titlebar_list = root_container->get_all_container_map_by_titlebar();

    if(titlebar_list.count(window))
    {
        const std::shared_ptr<EshyWMWindow> closed_window = titlebar_list.at(window)->get_window();
        titlebar_list.at(window)->get_window()->close_window();
    }
    else if (frame_list.count(window))
    {
        const std::shared_ptr<EshyWMWindow> closed_window = frame_list.at(window)->get_window();
        frame_list.at(window)->get_window()->close_window();
    }
}

void WindowManager::check_titlebar_button_pressed(Window window, int cursor_x, int cursor_y)
{
    const organized_container_map_t titlebar_list = root_container->get_all_container_map_by_titlebar();

    //This will check if the window is a frame (so we can't check the same area, but for a client window)
    if(titlebar_list.count(window))
    {
        const int button_pressed = titlebar_list.at(window)->get_window()->is_cursor_on_titlebar_buttons(window, cursor_x, cursor_y);

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