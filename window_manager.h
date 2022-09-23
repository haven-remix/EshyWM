
#pragma once

extern "C" {
#include <X11/Xlib.h>
}

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "util.h"
#include "window.h"

typedef std::unordered_map<Window, window_data*> window_map;

class WindowManager
{
public:

    static WindowManager* Create(const std::string& display_str = std::string());

    ~WindowManager();

    void Run();

    const Window get_root() const {return root;}

    int get_num_horizontal_slots() const {return num_horizontal_slots;}
    int get_num_vertical_slots() const {return num_vertical_slots;}

    window_map& get_window_list() {return window_list;}

private:

    WindowManager(Display* display);
    Display* display;
    const Window root;

    /**Current slots information*/
    int num_horizontal_slots;
    int num_vertical_slots;

    /**Mutex for protecting b_wm_detected*/
    static std::mutex mutex_wm_detected;

    /**Cursor position at the start of a window move/resize*/
    Position<int> drag_start_position;
    /**The position of the affected window at the start of a window*/
    Position<int> drag_start_frame_position;
    /**The size of the affected window at the start of a window move/resize*/
    size<int> drag_start_frame_size;

    /**Atom constants*/
    const Atom WM_PROTOCOLS;
    const Atom WM_DELETE_WINDOW;

    /**A list of windows, this will only contain the displayed window*/
    window_map window_list;

    /**Frame handlers*/
    void Frame(Window window, bool b_was_created_before_window_manager);
    void Unframe(Window window);

    /**Event handlers*/
    void OnCreateNotify(const XCreateWindowEvent& event);
    void OnDestroyNotify(const XDestroyWindowEvent& event);
    void OnReparentNotify(const XReparentEvent& event);
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

    /**Helper functions*/
    EshyWMWindow* register_window(Window window, bool b_was_created_before_window_manager);
    void unregister_window(Window window);
    /**Initializes the slot, location, and size*/
    void initialize_window(Window window, bool b_was_created_before_window_manager);
};