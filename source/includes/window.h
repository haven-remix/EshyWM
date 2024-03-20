#pragma once

#include <cairo/cairo.h>

#include "window_manager.h"
#include "util.h"

enum EWindowState : uint8_t
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

enum EWindowStateChangeCondition : uint8_t
{
    WSCC_MANUAL,
    WSCC_FROM_MOVE,
    WSCC_FROM_RESIZE,
    WSCC_FROM_MAXIMIZE,
    WSCC_FROM_FULLSCREEN,
    WSCC_STORE_STATE
};

/**
 * Handles everything about an individual window
*/
class EshyWMWindow : public std::enable_shared_from_this<EshyWMWindow>
{
public:

    EshyWMWindow(Window _window, bool _b_force_no_titlebar);
    ~EshyWMWindow();

    void frame_window(XWindowAttributes attr);
    void unframe_window();
    void setup_grab_events();
    void remove_grab_events();

    void toggle_minimize(void* null = nullptr) {minimize_window(get_window_state() != WS_MINIMIZED);}
    void toggle_maximize(void* null = nullptr) {maximize_window(get_window_state() != WS_MAXIMIZED);}
    void toggle_fullscreen(void* null = nullptr) {fullscreen_window(get_window_state() != WS_FULLSCREEN);}

    void minimize_window(bool b_minimize);
    void maximize_window(bool b_maximize);
    void fullscreen_window(bool b_fullscreen);
    void close_window(void* null = nullptr);
    void anchor_window(EWindowState anchor, std::shared_ptr<s_monitor_info> monitor_override = nullptr);
    void attempt_shift_monitor_anchor(EWindowState direction);
    void attempt_shift_monitor(EWindowState direction);

    void move_window_absolute(int new_position_x, int new_position_y, bool b_skip_state_checks);
    void resize_window_absolute(uint new_size_x, uint new_size_y, bool b_skip_state_checks);

    void update_titlebar();

    void set_show_titlebar(bool b_new_show_titlebar);

    /**Getters*/
    Window get_window() const {return window;}
    Window get_frame() const {return frame;}
    Window get_titlebar() const {return titlebar;}
    Rect get_frame_geometry() const {return frame_geometry;}
    Imlib_Image get_window_icon() const {return window_icon;}
    EWindowState get_window_state() const {return window_state;}
    void set_window_state(EWindowState new_window_state);

    std::shared_ptr<class WindowButton> get_close_button() const {return close_button;}

private:

    Window window;
    Window frame;
    Window titlebar;

    Rect frame_geometry;
    Rect pre_state_change_geometry;
    EWindowState previous_state;

    std::shared_ptr<s_monitor_info> current_monitor;

    EWindowState window_state;
    bool b_show_titlebar;
    bool b_force_no_titlebar;

    Imlib_Image window_icon;

    cairo_surface_t* cairo_titlebar_surface;
    cairo_t* cairo_context;

    std::shared_ptr<class WindowButton> close_button;
};