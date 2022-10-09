#pragma once

extern "C" {
#include <X11/Xlib.h>
}

#include "util.h"
#include "slot.h"

/**
 * Handles everything about an individual window
*/
class EshyWMWindow : public slot_child
{
public:

    EshyWMWindow(Window _window);

    int horizontal_position;
    int vertical_position;

    void frame_window(bool b_was_created_before_window_manager);
    void unframe_window();
    void setup_grab_events(bool b_was_created_before_window_manager);
    void draw_titlebar_buttons();
    void remove_grab_events();
    void close_window();

    void resize_window(Vector2D<int> delta);
    void resize_window_absolute(size<int> new_size);
    void resize_window_horizontal_left_arrow();
    void resize_window_horizontal_right_arrow();
    void resize_window_vertical_up_arrow();
    void resize_window_vertical_down_arrow();

    void recalculate_all_window_size_and_location();

    /**@return 0 = none; 1 = minimize; 2 = maximize; 3 = close*/
    int is_cursor_on_titlebar_buttons(Window window, int cursor_x, int cursor_y);

    /**Getters*/
    Window get_window() const {return window;}
    Window get_frame() const {return frame;}
    Window get_titlebar() const {return titlebar;}
    window_size_location_data get_frame_size_and_location_data() const {return frame_size_and_location_data;}
    window_size_location_data get_titlebar_size_and_location_data() const {return titlebar_size_and_location_data;}
    window_size_location_data get_window_size_and_location_data() const {return window_size_and_location_data;}
    size<int> get_preferred_size() const {return preferred_size;}

private:

    Window window;
    Window frame;
    Window titlebar;

    window_size_location_data frame_size_and_location_data;
    window_size_location_data titlebar_size_and_location_data;
    window_size_location_data window_size_and_location_data;

    size<int> preferred_size;

    GC graphics_context_internal;

    int get_resize_step_horizontal() const;
    int get_resize_step_vertical() const;
};