#pragma once

extern "C" {
#include <X11/Xlib.h>
}

#include "util.h"

enum class EScreenSlot
{
    SS_None,    //Used for floating
    SS_Left,
    SS_Right,
    SS_Top,
    SS_Bottom
};

class EshyWMWindow
{
public:

    EshyWMWindow(Display* _display, Window _window)
        : display(_display)
        , window(_window)
        , WindowSlot(EScreenSlot::SS_Right)
    {}

    void frame_window(bool b_was_created_before_window_manager);
    void unframe_window();

    void setup_grab_events(bool b_was_created_before_window_manager);

    void reisze_window_horizontal_left_arrow();
    void reisze_window_horizontal_right_arrow();
    void reisze_window_vertical_up_arrow();
    void reisze_window_vertical_down_arrow();

    EScreenSlot WindowSlot;

private:

    Display* display;
    Window window;
    Window frame;

    int get_resize_step_horizontal();
    int get_resize_step_vertical();

    window_data get_window_size();
};