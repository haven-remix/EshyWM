#pragma once

#include <Imlib2.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>

#include "window.h"
#include "util.h"

struct s_monitor_info
{
    s_monitor_info()
        : b_primary(false)
        , x(0)
        , y(0)
        , width(0)
        , height(0)
        , taskbar(nullptr)
    {}

    s_monitor_info(bool _b_primary, int _x, int _y, uint _width, uint _height)
        : b_primary(_b_primary)
        , x(_x)
        , y(_y)
        , width(_width)
        , height(_height)
        , taskbar(nullptr)
    {}
    
    bool b_primary;
    int x;
    int y;
    uint width;
    uint height;
    std::shared_ptr<class EshyWMTaskbar> taskbar;
};

class EshyWMWindow;

class WindowManager
{
public:

    void initialize();
    void handle_events();
    void handle_preexisting_windows();

    void focus_window(std::shared_ptr<EshyWMWindow> window);

    static Display* display;

    const std::vector<std::shared_ptr<EshyWMWindow>>& get_window_list() const {return window_list;}

private:

    std::vector<std::shared_ptr<EshyWMWindow>> window_list;

    //If user uses the minimize all function, then we store the minimized windows here to restore; since all the states will become minimized, we also need to store the state so we can restore it
    std::unordered_map<std::shared_ptr<EshyWMWindow>, EWindowState> view_desktop_window_list;

    Pos click_cursor_position;

    struct double_click_data
    {
        Window window;
        Time first_click_time;
        Time last_double_click_time;
    } titlebar_double_click;
    
    bool b_manipulating_with_keys = false;
    bool b_at_bottom_right_corner = false;

    std::shared_ptr<class Button> currently_hovered_button;
    Rect manipulating_window_geometry;

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

    std::shared_ptr<EshyWMWindow> window_list_contains_frame(Window frame);
    std::shared_ptr<EshyWMWindow> window_list_contains_titlebar(Window titlebar);
    std::shared_ptr<EshyWMWindow> window_list_contains_window(Window internal_window);
    void remove_from_window_list(std::shared_ptr<EshyWMWindow> window);

    std::shared_ptr<EshyWMWindow> register_window(Window window, bool b_was_created_before_window_manager);
    void handle_button_hovered(Window hovered_window, bool b_hovered, int mode);
};