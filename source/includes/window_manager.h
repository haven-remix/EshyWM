
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
#include "window_list_container.h"

typedef std::unordered_map<Window, EshyWMWindow*> window_map;

struct window_manager_data
{
    bool b_floating_mode;
};

class WindowManager
{
public:

    WindowManager();
    ~WindowManager();

    static std::shared_ptr<WindowManager> Create(const std::string& display_str = std::string());

    void Run();

    /**Getters*/
    static Display* get_display() {return display;}
    const Window get_root() {return root;}

    int get_num_horizontal_slots() {return num_horizontal_slots;}
    int get_num_vertical_slots() {return num_vertical_slots;}

    window_map& get_window_list() {return window_list;}

    const Atom get_atom_wm_protocols() {return WM_PROTOCOLS;}
    const Atom get_atom_wm_delete_window() {return WM_DELETE_WINDOW;}

private:

    static Display* display;
    const Window root;

    /**Current slots information*/
    int num_horizontal_slots;
    int num_vertical_slots;

    /**Mutex for protecting b_wm_detected*/
    static std::mutex mutex_wm_detected;

    /**Cursor position at the start of a window move/resize*/
    Vector2D<int> drag_start_position;

    /**Atom constants*/
    const Atom WM_PROTOCOLS;
    const Atom WM_DELETE_WINDOW;

    /**A list of windows, sorted by titlebar, frame, and window*/
    window_map window_list;
    window_map window_titlebar_list;

    /**Draws all drawables each frame*/
    void draw();

    /**Event handlers*/
    void OnDestroyNotify(const XDestroyWindowEvent& event);
    void OnUnmapNotify(const XUnmapEvent& event);
    void OnConfigureNotify(const XConfigureEvent& event);
    void OnMapRequest(const XMapRequestEvent& event);
    void OnConfigureRequest(const XConfigureRequestEvent& event);
    void OnButtonPress(const XButtonEvent& event);
    void OnMotionNotify(const XMotionEvent& event);
    void OnKeyPress(const XKeyEvent& event);

    /**Xlib error handler*/
    static int OnXError(Display* display, XErrorEvent* e);
    static int OnWMDetected(Display* display, XErrorEvent* e);
    static bool b_wm_detected;

    /**Helper functions*/
    EshyWMWindow* register_window(Window window, bool b_was_created_before_window_manager);
    void unregister_window(Window window);
    void close_window(Window window);
    /**Initializes the slot, location, and size*/
    void initialize_window(Window window, bool b_was_created_before_window_manager);

    void check_titlebar_button_pressed(Window window, int cursor_x, int cursor_y);
};