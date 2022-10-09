
#pragma once

extern "C" {
#include <X11/Xlib.h>
}

#include <memory>
#include <mutex>
#include <string>
#include <map>

#include "util.h"
#include "window.h"

typedef std::map<Window, EshyWMWindow*> window_map;

enum class EWindowTileMode
{
    WTM_None,
    WTM_Equal,          //Space everything equally
    WTM_Adjustive,      //Changing size only affects focused window, rest only compensate (make space, claim space)
    WTM_Seperate        //Every window is its own entity and do not effect each other
};

struct window_manager_data
{
    bool b_tiling_mode;
    EWindowTileMode window_tile_mode;

    window_manager_data()
        : b_tiling_mode(true)
        , window_tile_mode(EWindowTileMode::WTM_Equal)
    {}
};

class WindowManager
{
public:

    WindowManager();
    ~WindowManager();

    static std::shared_ptr<WindowManager> Create(const std::string& display_str = std::string());

    void Run();

    void window_size_updated(EshyWMWindow* window);

    /**Getters*/
    static Display* get_display() {return display;}
    const Window get_root() {return root;}

    window_map& get_frame_list() {return window_frame_list;}
    window_map& get_titlebar_list() {return window_titlebar_list;}

    window_manager_data* get_manager_data() const {return manager_data;}

    const Atom get_atom_wm_protocols() {return WM_PROTOCOLS;}
    const Atom get_atom_wm_delete_window() {return WM_DELETE_WINDOW;}

private:

    static Display* display;
    const Window root;

    /**Mutex for protecting b_wm_detected*/
    static std::mutex mutex_wm_detected;

    /**Cursor position at the start of a window move/resize*/
    Vector2D<int> drag_start_position;

    /**Atom constants*/
    const Atom WM_PROTOCOLS;
    const Atom WM_DELETE_WINDOW;

    /**A list of windows, sorted by titlebar, frame*/
    window_map window_frame_list;
    window_map window_titlebar_list;

    window_manager_data* manager_data;

    slot* main_slot;
    slot* current_slot;

    void main_loop();

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
    void close_window(Window window);
    /**Initializes the slot, location, and size*/
    void initialize_window(EshyWMWindow* window, bool b_was_created_before_window_manager);

    void check_titlebar_button_pressed(Window window, int cursor_x, int cursor_y);
};