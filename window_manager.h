
#pragma once

extern "C" {
#include <X11/Xlib.h>
}

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "util.h"

class WindowManager
{
public:

    static ::std::unique_ptr<WindowManager> Create(const std::string& display_str = std::string());

    ~WindowManager();

    void Run();

private:

    WindowManager(Display* display);
    Display* display;
    const Window root;
    std::unordered_map<Window, Window> clients;

    /**Frame handlers*/
    void Frame(Window window, bool b_was_created_before_window_manager);
    void Unframe(Window window);

    /**Event handlers*/
    void OnCreateNotify(const XCreateWindowEvent& event);
    void OnDestroyNotify(const XDestroyWindowEvent& event);
    void OnReparentNotify(const XDestroyWindowEvent& event);
    void OnMapNotify(const XMapEvent& event);
    void OnUnmapNotify(const XUnmapEvent& event);
    void OnConfigureNotify(const XConfigureEvent& event);
    void OnMapRequest(const XMapRequestEvent& event);
    void OnConfigureRequest(const XConfigureRequestEvent& event);
    void OnButtonPress(const XButtonEvent& event);
    void OnButtonRelease(const XButtonEvent& event);
    void OnMotionNotify(const XMotionEvent& event);
    void OnKeyPress(const XKeyEvent& event);
    void OnKeyRelease(const XKeyEvent& event);

    /**Xlib error handler*/
    static int OnXError(Display* display, XErrorEvent* e);
    static int OnWMDetected(Display* display, XErrorEvent* e);
    static bool b_wm_detected;

    /**Mutex for protecting b_vm_detected*/
    static std::mutex mutex_vm_detexted;

    /**Cursor position at the start of a window move/resize*/
    Position<int> drag_start_position;
    /**The position of the affected window at the start of a window*/
    Position<int> drag_start_frame_position;
    /**The size of the affected window at the start of a window move/resize*/
    Size<int> drag_start_frame_size;

    /**Atom constants*/
    const Atom WM_PROTOCOLS;
    const Atom WM_DELETE_WINDOW;
};