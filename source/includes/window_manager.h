
#pragma once

#include <Imlib2.h>
#include <memory>
#include <mutex>
#include <string>
#include <map>

#include "util.h"
#include "container.h"

enum class EWindowTileMode
{
    WTM_None,
    WTM_Equal,          //Space everything equally
    WTM_Adjustive,      //Changing size only affects focused window, rest only compensate (make space, claim space)
    WTM_Seperate        //Every window is its own entity and do not effect each other
};

struct window_manager_data
{
    bool b_tiling_mode;
    EWindowTileMode window_tile_mode;

    window_manager_data()
        : b_tiling_mode(false)
        , window_tile_mode(EWindowTileMode::WTM_Equal)
    {}
};

struct double_click_data
{
    Window first_click_window;
    Time first_click_time;
};

namespace WindowManager
{
    void initialize();
    void main_loop();
    void handle_preexisting_windows();

    /**Helper functions*/
    std::shared_ptr<class EshyWMWindow> register_window(Window window, bool b_was_created_before_window_manager);
    void close_window(std::shared_ptr<class EshyWMWindow> closed_window);
    void rescale_windows(uint old_width, uint old_height);
    void check_titlebar_button_pressed(Window window, int cursor_x, int cursor_y);

namespace Internal
{
    extern Display* display;

    extern Vector2D<int> click_cursor_position;
    extern std::shared_ptr<class container> root_container;
    extern window_manager_data* manager_data;

    extern uint display_width;
    extern uint display_height;

    extern organized_container_map_t frame_list;
    extern organized_container_map_t titlebar_list;

    extern double_click_data titlebar_double_click;
};
};