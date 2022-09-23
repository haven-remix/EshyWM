#pragma once

extern "C" {
#include <X11/Xlib.h>
}

#include "util.h"
#include "window_data.h"

class EshyWMWindow
{
public:

    EshyWMWindow(Display* _display, Window _window)
        : display(_display)
        , window(_window)
        , slot_data(nullptr)
    {}

    void frame_window(bool b_was_created_before_window_manager);
    void unframe_window();

    void setup_grab_events(bool b_was_created_before_window_manager);

    void set_window_data(window_data* new_window_data) {slot_data = new_window_data;}

    void resize_window_horizontal_left_arrow();
    void resize_window_horizontal_right_arrow();
    void resize_window_vertical_up_arrow();
    void resize_window_vertical_down_arrow();

private:

    Display* display;
    Window window;
    Window frame;

    window_data* slot_data;

    int get_resize_step_horizontal();
    int get_resize_step_vertical();

    window_size_data get_window_size();
};