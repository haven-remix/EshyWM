#pragma once

#include "window_manager.h"
#include "util.h"

#include <Imlib2.h>
#include <cairo/cairo.h>

typedef Window XWindow;

enum EWindowState : uint16_t
{
    WS_NONE,
    WS_NORMAL,
    WS_MINIMIZED,
    WS_MAXIMIZED,
    WS_FULLSCREEN,
    WS_ANCHORED_LEFT,
    WS_ANCHORED_UP,
    WS_ANCHORED_RIGHT,
    WS_ANCHORED_DOWN
};

/**
 * Handles everything about an individual window
*/
class EshyWMWindow
{
public:

    EshyWMWindow(Window _window, struct Workspace* _workspace, const Rect& geometry);
    ~EshyWMWindow();

    void frame_window();
    void unframe_window();

    void toggle_minimize() {minimize_window(get_window_state() != WS_MINIMIZED);}
    void toggle_maximize() {maximize_window(get_window_state() != WS_MAXIMIZED);}
    void toggle_fullscreen() {fullscreen_window(get_window_state() != WS_FULLSCREEN);}

    //Window states
    void set_window_state(EWindowState new_window_state);
    void minimize_window(bool b_minimize);
    void maximize_window(bool b_maximize);
    void fullscreen_window(bool b_fullscreen);
    void close_window();

    void anchor_window(EWindowState anchor, Output* output_override = nullptr);
    void attempt_shift_monitor_anchor(EWindowState direction);
    void attempt_shift_monitor(EWindowState direction);

    void move_window_absolute(int new_position_x, int new_position_y, bool b_skip_state_checks);
    void resize_window_absolute(uint new_size_x, uint new_size_y, bool b_skip_state_checks);

    void update_titlebar();

    void set_show_titlebar(bool b_new_show_titlebar);
    void set_show_border(bool b_show_border);

    /**Getters*/
    inline const Window get_window() const {return window;}
    inline const Window get_frame() const {return frame;}
    inline const Window get_titlebar() const {return titlebar;}
    inline const Rect& get_frame_geometry() const {return frame_geometry;}
    inline const Imlib_Image get_window_icon() const {return window_icon;}
    inline const EWindowState get_window_state() const {return window_state;}

    inline class WindowButton* get_close_button() const {return close_button;}

    Workspace* parent_workspace;

    bool b_show_titlebar;

private:

    Window window;
    Window frame;
    Window titlebar;

    Rect frame_geometry;
    Rect pre_state_change_geometry;
    EWindowState previous_state;

    EWindowState window_state;

    Imlib_Image window_icon;

    cairo_surface_t* cairo_titlebar_surface;
    cairo_t* cairo_context;

    class WindowButton* close_button;
};