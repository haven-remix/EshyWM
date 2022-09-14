#include "window_manager.h"
#include "eshywm.h"

extern "C" {
#include <X11/Xutil.h>
}

#include <glog/logging.h>
#include <cstring>
#include <iostream>
#include <algorithm>

bool WindowManager::b_wm_detected;
std::mutex WindowManager::mutex_wm_detected;

std::unique_ptr<WindowManager> WindowManager::Create(const std::string& display_str)
{
    Display* display = XOpenDisplay(display_str.empty() ? nullptr : display_str.c_str());

    if(display == nullptr)
    {
        LOG(ERROR) << "Failed to open X display " << XDisplayName(nullptr);
        return nullptr;
    }

    return std::unique_ptr<WindowManager>(new WindowManager(display));
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

    XSetErrorHandler(&WindowManager::OnXError);
    //Grab X server to prevent windows from changing under us
    XGrabServer(display);

    Window returned_root;
    Window returned_parent;
    Window* top_level_windows;

    unsigned int num_top_level_windows;
    CHECK(XQueryTree(display, root, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows));
    CHECK_EQ(returned_root, root);

    //Frame each top level window
    for(unsigned int i = 0; i < num_top_level_windows; ++i)
    {
        Frame(top_level_windows[i], true);
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
        LOG(INFO) << "Received event: " << ToString(event);

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

}

void WindowManager::OnReparentNotify(const XReparentEvent& event)
{

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
    
}

void WindowManager::OnUnmapNotify(const XUnmapEvent& event)
{
    //If we manage window then unframe. Need to check bcause we will receive an UnmapNotify event for a fram window we just destroyed
    if(!clients.count(event.window))
    {
        LOG(INFO) << "Ignore UnmapNotify for non-client window " << event.window;
        return;
    }

    //Ignore event if it is triggered by reparenting a window that was mapped before the window manager started
    if(event.event == root)
    {
        LOG(INFO) << "Ignore UnmapNotify for reparented pre-existing window " << event.window;
        return;
    }

    Unframe(event.window);
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

    if(clients.count(event.window))
    {
        const Window frame = clients[event.window];
        XConfigureWindow(display, frame, event.value_mask, &changes);
        LOG(INFO) << "Resize [" << frame << "] to " << Size<int>(event.width, event.height);
    }

    //Grant request
    XConfigureWindow(display, event.window, event.value_mask, &changes);
    LOG(INFO) << "Resize " << event.window << " to " << Size<int>(event.width, event.height);
}

void WindowManager::OnMapRequest(const XMapRequestEvent& event)
{
    //Frame or reframe window
    Frame(event.window, false);
    //Map window
    XMapWindow(display, event.window);
}

void WindowManager::Frame(Window window, bool b_was_created_before_window_manager)
{
    //Visual properties of the frame
    const unsigned int BORDER_WIDTH = 0;
    const unsigned long BORDER_COLOR = 0xc461e8;
    const unsigned long BG_COLOR = 0xc461e8;

    //Retrieve attributes of window to frame
    XWindowAttributes x_window_attributes = {0};
    CHECK(XGetWindowAttributes(display, window, &x_window_attributes));

    //If window was created before window manager started, we should frame it only if it is visible and does not set override_redirect
    if(b_was_created_before_window_manager)
    {
        if(x_window_attributes.override_redirect || x_window_attributes.map_state != IsViewable)
        {
            return;
        }
    }

    //Create frame
    const Window frame = XCreateSimpleWindow(display, root, x_window_attributes.x, x_window_attributes.y, x_window_attributes.width, x_window_attributes.height, BORDER_WIDTH, BORDER_COLOR, BG_COLOR);

    //Select events on frame
    XSelectInput(display, frame, SubstructureRedirectMask | SubstructureNotifyMask);

    //Add client to save set, so that it will be restored and kept alive if we crash
    XAddToSaveSet(display, window);

    //Reparent client window. Note: last params are offsets of client window within frame
    XReparentWindow(display, window, frame, 0, 0);

    //Map frame
    XMapWindow(display, frame);

    //Save frame handle
    clients[window] = frame;

    //Grab events for window management actions on client window
    //Raise windows with left button
    XGrabButton(display, Button1, 0, window, false, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    //Move windows with alt + left button
    XGrabButton(display, Button1, Mod1Mask, window, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    //Resize windows with alt + right button
    XGrabButton(display, Button3, Mod1Mask, window, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    //Kill windows with alt + f4
    XGrabKey(display, XKeysymToKeycode(display, XK_F4), Mod1Mask, window, false, GrabModeAsync, GrabModeAsync);
    //Switch windows with alt + tab
    XGrabKey(display, XKeysymToKeycode(display, XK_Tab), Mod1Mask, window, false, GrabModeAsync, GrabModeAsync);

    LOG(INFO) << "Framed window " << window << " [" << frame << "]";
}

void WindowManager::Unframe(Window window)
{
    CHECK(clients.count(window));

    //Reverse what we did in frame
    const Window frame = clients[window];
    //Unmap frame
    XUnmapWindow(display, frame);
    //Reparent client window back to root window. Last params are the offset of the client window within root
    XReparentWindow(display, window, root, 0, 0);
    //Remove client window from save set, as it is now unrelated to us.
    XRemoveFromSaveSet(display, window);
    //Destroy frame
    XDestroyWindow(display, frame);
    //Drop referrence to frame handle
    clients.erase(window);

    LOG(INFO) << "Unframed window " << window << " [" << frame << "]";
}

void WindowManager::OnButtonPress(const XButtonEvent& event)
{
    CHECK(clients.count(event.window));
    const Window frame = clients[event.window];

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
    drag_start_frame_size = Size<int>(width, height);

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
    CHECK(clients.count(event.window));
    const Window frame = clients[event.window];
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
        const Size<int> dest_frame_size = drag_start_frame_size + size_delta;

        //Resize frame
        XResizeWindow(display, frame, dest_frame_size.width, dest_frame_size.height);

        //Resize client window
        XResizeWindow(display, event.window, dest_frame_size.width, dest_frame_size.height);
    }
}

void WindowManager::OnKeyPress(const XKeyEvent& event)
{
    if ((event.state & Mod1Mask) && (event.keycode == XKeysymToKeycode(display, XK_F4)))
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
    else if ((event.state & Mod1Mask) & (event.keycode == XKeysymToKeycode(display, XK_Tab)))
    {
        //Find next window
        auto i = clients.find(event.window);
        CHECK(i != clients.end());
        i++;
        if(i == clients.end())
        {
            i = clients.begin();
        }
        //Raise and set focus
        XRaiseWindow(display, i->second);
        XSetInputFocus(display, i->first, RevertToPointerRoot, CurrentTime);
    }
}

void WindowManager::OnKeyRelease(const XKeyEvent& event)
{
    
}