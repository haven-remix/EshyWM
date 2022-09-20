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

WindowManager* WindowManager::Create(const std::string& display_str)
{
    Display* display = XOpenDisplay(display_str.empty() ? nullptr : display_str.c_str());

    if(display == nullptr)
    {
        LOG(ERROR) << "Failed to open X display " << XDisplayName(nullptr);
        return nullptr;
    }

    return new WindowManager(display);
}

WindowManager::WindowManager(Display* display) 
    : display(CHECK_NOTNULL(display))
    , root(DefaultRootWindow(display))
    , WM_PROTOCOLS(XInternAtom(display, "WM_PROTOCOLS", false))
    , WM_DELETE_WINDOW(XInternAtom(display, "WM_DELETE_WINDOW", false))
{
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
    CHECK(XQueryTree(display, root, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows));
    CHECK_EQ(returned_root, root);

    //Register, frame, and setup grab events for each top level window
    for(unsigned int i = 0; i < num_top_level_windows; ++i)
    {
        EshyWMWindow* window = register_window(top_level_windows[i]);
        window->frame_window(true);
        window->setup_grab_events(true);
    }

    //Free top-level window array
    XFree(top_level_windows);

    //Ungrab X server
    XUngrabServer(display);

    //Main event loop
    for (;;)
    {
        //Get next event
        XEvent event;
        XNextEvent(display, &event);
        LOG(INFO) << "Received event: " << to_string(event);

        //Dispatch event
        switch (event.type)
        {
        case CreateNotify:
            OnCreateNotify(event.xcreatewindow);
            break;
        case DestroyNotify:
            OnDestroyNotify(event.xdestroywindow);
            break;
        case ReparentNotify:
            OnReparentNotify(event.xreparent);
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
        case MapRequest:
            OnMapRequest(event.xmaprequest);
            break;
        case ConfigureRequest:
            OnConfigureRequest(event.xconfigurerequest);
            break;
        case ButtonPress:
            OnButtonPress(event.xbutton);
            break;
        case ButtonRelease:
            OnButtonRelease(event.xbutton);
            break;
        case MotionNotify:
            while (XCheckTypedWindowEvent(display, event.xmotion.window, MotionNotify, &event)) {}
            OnMotionNotify(event.xmotion);
            break;
        case KeyPress:
            OnKeyPress(event.xkey);
            break;
        case KeyRelease:
            OnKeyRelease(event.xkey);
            break;
        default:
            LOG(WARNING) << "Ignored event";
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

void WindowManager::OnCreateNotify(const XCreateWindowEvent& event)
{
    
}

void WindowManager::OnDestroyNotify(const XDestroyWindowEvent& event)
{
    for(auto it = window_list.begin(); it != window_list.end(); ++it)
    {
        if(it->first == event.window)
        {
            window_list.erase(it++);
            break;
        }
    }
}

void WindowManager::OnReparentNotify(const XReparentEvent& event)
{
    //std::cout << "reparent" << std::endl;
}

void WindowManager::OnConfigureNotify(const XConfigureEvent& event)
{
    if(event.window == root)
    {
        //Notify screen resolution changed
        EshyWM::on_screen_resolution_changed();
    }
}

void WindowManager::OnMapNotify(const XMapEvent& event)
{
    //std::cout << "map notify" << std::endl;
}

void WindowManager::OnUnmapNotify(const XUnmapEvent& event)
{
    //If we manage window then unframe. Need to check bcause we will receive an UnmapNotify event for a fram window we just destroyed
    if(!window_list.count(event.window))
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

    window_list[event.window]->unframe_window();
}

void WindowManager::OnConfigureRequest(const XConfigureRequestEvent& event)
{
    std::cout << "cr" << std::endl;

    XWindowChanges changes;
    changes.x = event.x;
    changes.y = event.y;
    changes.width = event.width;
    changes.height = event.height;
    changes.border_width = event.border_width;
    changes.sibling = event.above;
    changes.stack_mode = event.detail;

    if(window_list.count(event.window))
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
    register_window(event.window);
    //Frame or reframe window and setup grap events
    window_list[event.window]->frame_window(false);
    window_list[event.window]->setup_grab_events(false);
    //Map window
    XMapWindow(display, event.window);
}

void WindowManager::OnButtonPress(const XButtonEvent& event)
{
    CHECK(window_list[event.window]);
    const Window frame = event.window;

    //Save initial cursor position
    drag_start_position = Position<int>(event.x_root, event.y_root);

    //Save initial window info
    Window returned_root;
    int x;
    int y;
    unsigned width;
    unsigned height;
    unsigned border_width;
    unsigned depth;

    CHECK(XGetGeometry(display, frame, &returned_root, &x, &y, &width, &height, &border_width, &depth));
    drag_start_frame_position = Position<int>(x, y);
    drag_start_frame_size = size<int>(width, height);

    //Raise clicked window to top and set focus
    XRaiseWindow(display, frame);
    XSetInputFocus(display, frame, RevertToPointerRoot, event.time);
    //Pass the click event through
    XAllowEvents(display, ReplayPointer, event.time);
}

void WindowManager::OnButtonRelease(const XButtonEvent& event)
{

}

void WindowManager::OnMotionNotify(const XMotionEvent& event)
{
    CHECK(window_list.count(event.window));
    const Window frame = event.window;
    const Position<int> drag_position(event.x_root, event.y_root);
    const Vector2D<int> delta = drag_position - drag_start_position;

    if(event.state & Button1Mask)
    {
        const Position<int> dest_frame_position = drag_start_frame_position + delta;
        XMoveWindow(display, frame, dest_frame_position.x, dest_frame_position.y);
    }
    else if(event.state & Button3Mask)
    {
        const Vector2D<int> size_delta(std::max(delta.x, -drag_start_frame_size.width), std::max(delta.y, -drag_start_frame_size.height));
        const size<int> dest_frame_size = drag_start_frame_size + size_delta;

        //Resize frame
        XResizeWindow(display, frame, dest_frame_size.width, dest_frame_size.height);

        //Resize client window
        XResizeWindow(display, event.window, dest_frame_size.width, dest_frame_size.height);
    }
}

void WindowManager::OnKeyPress(const XKeyEvent& event)
{
    if ((event.state & Mod1Mask) && (event.keycode == XKeysymToKeycode(display, XK_C)))
    {
        Atom* supported_protocols;
        int num_supported_protocols;

        if (XGetWMProtocols(display, event.window, &supported_protocols, &num_supported_protocols) && (std::find(supported_protocols, supported_protocols + num_supported_protocols, WM_DELETE_WINDOW) != supported_protocols + num_supported_protocols))
        {
            LOG(INFO) << "Gracefully deleting window " << event.window;

            //Construct message
            XEvent message;
            memset(&message, 0, sizeof(message));
            message.xclient.type = ClientMessage;
            message.xclient.message_type = WM_PROTOCOLS;
            message.xclient.window = event.window;
            message.xclient.format = 32;
            message.xclient.data.l[0] = WM_DELETE_WINDOW;

            //Send message to window to be closed
            CHECK(XSendEvent(display, event.window, false, 0 , &message));
        }
        else
        {
            LOG(INFO) << "Killing window " << event.window;
            XKillClient(display, event.window);
        }
    }
    else if ((event.state & Mod1Mask) && (event.keycode == XKeysymToKeycode(display, XK_Tab)))
    {
        //Find next window
        auto i = window_list.find(event.window);
        CHECK(i != window_list.end());
        i++;
        if(i == window_list.end())
        {
            i = window_list.begin();
        }
        
        //Raise and set focus
        XRaiseWindow(display, i->first);
        XSetInputFocus(display, i->first, RevertToPointerRoot, CurrentTime);
    }
    else if((event.state & Mod1Mask) && (event.keycode == XKeysymToKeycode(display, XK_Left)))
    {
        window_list[event.window]->reisze_window_horizontal_left_arrow();
    }
    else if((event.state & Mod1Mask) && (event.keycode == XKeysymToKeycode(display, XK_Right)))
    {
        window_list[event.window]->reisze_window_horizontal_right_arrow();
    }
    else if((event.state & Mod1Mask) && (event.keycode == XKeysymToKeycode(display, XK_Up)))
    {
        window_list[event.window]->reisze_window_vertical_up_arrow();
    }
    else if((event.state & Mod1Mask) && (event.keycode == XKeysymToKeycode(display, XK_Down)))
    {
        window_list[event.window]->reisze_window_vertical_down_arrow();
    }
}

void WindowManager::OnKeyRelease(const XKeyEvent& event)
{
    
}


EshyWMWindow* WindowManager::register_window(Window window)
{
    EshyWMWindow* new_window = new EshyWMWindow(display, window);
    window_list.emplace(window, new_window);
    return new_window;
}
