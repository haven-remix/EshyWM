#pragma once

extern "C" {
#include <X11/Xlib.h>
}

#include "util.h"

/**
 * Handles everything about an individual window
*/
class EshyWMWindow
{
public:

    EshyWMWindow(Window _window)
        : window(_window)
        , b_is_maximized(false)
        , b_is_minimized(false)
    {}

    void frame_window(bool b_was_created_before_window_manager);
    void unframe_window();
    void setup_grab_events(bool b_was_created_before_window_manager);
    void remove_grab_events();
    void minimize_window();
    void maximize_window(bool b_from_move_or_resize = false);
    void close_window();

    void move_window(Vector2D<int> delta) {move_window(delta.x, delta.y);}
    void move_window_absolute(Vector2D<int> new_position) {move_window_absolute(new_position.x, new_position.y);}
    void move_window(int delta_x, int delta_y);
    void move_window_absolute(int new_position_x, int new_position_y);
    void resize_window(Vector2D<int> delta) {resize_window(delta.x, delta.y);}
    void resize_window_absolute(Vector2D<uint> new_size) {resize_window_absolute(new_size.x, new_size.y);}
    void resize_window(int delta_x, int delta_y);
    void resize_window_absolute(uint new_size_x, uint new_position_y);
    void motion_modify_ended();
    void recalculate_all_window_size_and_location();

    void draw_titlebar();

    /**@return 0 = none; 1 = minimize; 2 = maximize; 3 = close*/
    int is_cursor_on_titlebar_buttons(Window window, int cursor_x, int cursor_y);

    /**Getters*/
    Window get_window() const {return window;}
    Window get_frame() const {return frame;}
    Window get_titlebar() const {return titlebar;}
    rect get_frame_size_and_location_data() const {return frame_size_and_location_data;}
    rect get_titlebar_size_and_location_data() const {return titlebar_size_and_location_data;}
    rect get_window_size_and_location_data() const {return window_size_and_location_data;}
    bool is_minimized() const {return b_is_minimized;}

private:

    Window window;
    Window frame;
    Window titlebar;

    rect frame_size_and_location_data;
    rect titlebar_size_and_location_data;
    rect window_size_and_location_data;
    rect pre_minimize_and_maximize_saved_size_and_location_data;
    rect temp_move_and_resize_size_and_location_data;

    bool b_is_maximized;
    bool b_is_minimized;
    bool b_is_currently_moving_or_resizing;

    GC graphics_context_internal;

    std::shared_ptr<class Button> minimize_button;
    std::shared_ptr<class Button> maximize_button;
    std::shared_ptr<class Button> close_button;
};