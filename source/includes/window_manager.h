#pragma once

#include "window.h"
#include "util.h"
#include "container.h"
#include "config.h"

#include <Imlib2.h>

#include <vector>

/**
 * The window manager layout is as follows.
 * WindowManager -> {Outputs, Workspaces}
 * Workspaces -> Windows
 * Outputs -> {Docks, Active Workspace} 
 * 
 * In WindowManager, we store a list of outputs and workspaces.
 * Each output will have a top and bottom dock and space for the active workspace.
 * 
 * The Workspace list is not attached to any output. Only the active workspace is.
 * This means any output can grab any workspace as it choses, but only one at a time.
 * 
 * An output represents a monitor. A workspace is an arbitrary collection of windows used
 * primarily for organization.
 * 
 * The WindowManager class only manages the interaction between windows, user input,
 * system events, and NOT the windows' usage.
 * 
 * The Window class manages the usage of the window (resizing, moving, closing, etc.).
 * That said, the WindowManager is responsible for relaying those events to the window.
 * 
 * X11 is a file of utility classes to C++-ify X code to make its usage easier and more consistent.
 * 
 * The System namespace is for managing pure system things like battery, time, etc.
 * 
 * The Utils file defines common data structures (Pos, Rect, etc.) and common functionality (retrieving
 * cursor position, finding position to center the window on the screen, etc.)
 * 
 * The Config file defines configuration. The configuration is pulled from the eshywm.conf file at startup.
 * The configuration can be changed temporary through usage. For example, titlebars can be hidden using
 * a hotkey. These changes only last the duration of the session. Permanent changes will soon be supported
 * with some kind of settings menu. Perhaps a configuration retention feature will also be added so all
 * session changes will remain across startups.
*/
class WindowManager
{
public:

    WindowManager()
        : titlebar_double_click{0, 0, 0}
        , click_cursor_position{0, 0}
        , focused_window(nullptr)
        , currently_hovered_button(nullptr)
        , b_manipulating_with_keys(false)
        , b_manipulating_with_titlebar(false)
        , b_show_window_borders(EshyWMConfig::window_frame_border_width != 0)
    {}

    void initialize();
    void handle_events();
    void handle_preexisting_windows();

    //Uses XRandR to scan outputs and generates relevant Outputs
    void scan_outputs();

    void focus_window(std::shared_ptr<EshyWMWindow> window, bool b_raise);

    std::vector<std::shared_ptr<Output>> outputs;
    std::vector<std::shared_ptr<Workspace>> workspaces;
    std::vector<std::shared_ptr<EshyWMWindow>> window_list;

    std::shared_ptr<EshyWMWindow> focused_window;

    bool b_show_window_borders;

private:

    Pos click_cursor_position;

    struct double_click_data
    {
        std::shared_ptr<EshyWMWindow> window;
        Time first_click_time;
        Time last_double_click_time;
    } titlebar_double_click;
    
    bool b_manipulating_with_keys = false;
    bool b_manipulating_with_titlebar = false;

    class Button* currently_hovered_button;
    Rect manipulating_window_geometry;

    void grab_keys();
    void ungrab_keys();

    void OnDestroyNotify(const XDestroyWindowEvent& event);
    void OnMapNotify(const XMapEvent& event);
    void OnUnmapNotify(const XUnmapEvent& event);
    void OnMapRequest(const XMapRequestEvent& event);
    void OnPropertyNotify(const XPropertyEvent& event);
    void OnConfigureNotify(const XConfigureEvent& event);
    void OnConfigureRequest(const XConfigureRequestEvent& event);
    void OnVisibilityNotify(const XVisibilityEvent& event);
    void OnButtonPress(const XButtonEvent& event);
    void OnButtonRelease(const XButtonEvent& event);
    void OnMotionNotify(const XMotionEvent& event);
    void OnKeyPress(const XKeyEvent& event);
    void OnKeyRelease(const XKeyEvent& event);
    void OnEnterNotify(const XCrossingEvent& event);
    void OnClientMessage(const XClientMessageEvent& event);

    std::shared_ptr<EshyWMWindow> register_window(Window window, bool b_was_created_before_window_manager);
    std::shared_ptr<Dock> register_dock(Window window, bool b_was_created_before_window_manager);

    void handle_button_hovered(Window hovered_window, bool b_hovered, int mode);

    std::shared_ptr<EshyWMWindow> contains_xwindow(Window window);
};