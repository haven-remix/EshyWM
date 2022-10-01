#pragma once

extern "C" {
#include <X11/Xlib.h>
}

#include "util.h"
#include "window_position_data.h"

/**
 * Handles everything about an individual window
*/
class EshyWMWindow
{
public:

    EshyWMWindow(Window _window);

    EshyWMWindow(Window _window, window_position_data* _position_data)
        : window(_window)
        , position_data(_position_data)
    {}

    void frame_window(bool b_was_created_before_window_manager);
    void unframe_window();

    void setup_grab_events(bool b_was_created_before_window_manager);

    void resize_window_horizontal_left_arrow();
    void resize_window_horizontal_right_arrow();
    void resize_window_vertical_up_arrow();
    void resize_window_vertical_down_arrow();

    /**Getters*/
    Window get_window() {return window;}
    Window get_frame() {return frame;}
    window_position_data* get_widnow_position_data() {return position_data;}

    /**Setters*/
    void set_window_position_data(window_position_data* new_window_position_data) {position_data = new_window_position_data;}

private:

    Window window;
    Window frame;

    window_position_data* position_data;

    window_size_data get_window_size();

    int get_resize_step_horizontal();
    int get_resize_step_vertical();
};