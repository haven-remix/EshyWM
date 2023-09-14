#pragma once

#include <cairo/cairo.h>

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

    static void toggle_minimize(std::shared_ptr<class EshyWMWindow> window, void* null = nullptr) {window->minimize_window(window->get_window_state() != WS_MINIMIZED);}
    static void toggle_maximize(std::shared_ptr<class EshyWMWindow> window, void* null = nullptr) {window->maximize_window(window->get_window_state() != WS_MAXIMIZED);}
    static void toggle_fullscreen(std::shared_ptr<class EshyWMWindow> window, void* null = nullptr) {window->fullscreen_window(window->get_window_state() != WS_FULLSCREEN);}

    void minimize_window(bool b_minimize, EWindowStateChangeCondition condition = WSCC_MANUAL);
    void maximize_window(bool b_maximize, EWindowStateChangeCondition condition = WSCC_MANUAL);
    void fullscreen_window(bool b_fullscreen, EWindowStateChangeCondition condition = WSCC_MANUAL);
    static void close_window(std::shared_ptr<class EshyWMWindow> window, void* null = nullptr);
    static void focus_window(std::shared_ptr<class EshyWMWindow> window);
    void anchor_window(EWindowState anchor, std::shared_ptr<s_monitor_info> monitor_override = nullptr);
    void attempt_shift_monitor_anchor(EWindowState direction);
    void attempt_shift_monitor(EWindowState direction);

    void move_window_absolute(int new_position_x, int new_position_y, bool b_from_maximize = false);
    void resize_window_absolute(uint new_size_x, uint new_size_y, bool b_from_maximize = false);

    void update_titlebar();

    void set_show_titlebar(bool b_new_show_titlebar);

    /**Getters*/
    Window get_window() const {return window;}
    Window get_frame() const {return frame;}
    Window get_titlebar() const {return titlebar;}
    rect get_frame_geometry() const {return frame_geometry;}
    Imlib_Image get_window_icon() const {return window_icon;}
    EWindowState get_window_state() const {return window_state;}
    void set_window_state(EWindowState new_window_state);

    std::shared_ptr<class WindowButton> get_minimize_button() const {return minimize_button;}
    std::shared_ptr<class WindowButton> get_maximize_button() const {return maximize_button;}
    std::shared_ptr<class WindowButton> get_close_button() const {return close_button;}

public:

    Window window;
    Window frame;
    Window titlebar;

    rect frame_geometry;
    rect pre_state_change_geometry;
    EWindowState previous_state;

    std::shared_ptr<s_monitor_info> current_monitor;

    EWindowState window_state;
    bool b_show_titlebar;
    bool b_force_no_titlebar;

    Imlib_Image window_icon;

    cairo_surface_t* cairo_titlebar_surface;
    cairo_t* cairo_context;

    std::shared_ptr<class WindowButton> minimize_button;
    std::shared_ptr<class WindowButton> maximize_button;
    std::shared_ptr<class WindowButton> close_button;
};