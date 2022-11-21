#pragma once

#include <string>

namespace EshyWMConfig
{
    /**Window frame*/
    extern uint window_frame_border_width;
    extern ulong window_frame_border_color;
    extern ulong window_background_color;

    /**Titlebar*/
    extern uint titlebar_height;
    extern uint titlebar_button_size;
    extern uint titlebar_button_padding;
    extern ulong titlebar_button_normal_color;
    extern ulong titlebar_button_hovered_color;
    extern ulong titlebar_button_pressed_color;
    extern ulong titlebar_title_color;

    /**Context menu*/
    extern uint context_menu_width;
    extern ulong context_menu_color;
    extern ulong context_menu_secondary_color;

    /**Taskbar*/
    extern uint taskbar_height;
    extern ulong taskbar_color;

    /**Switcher*/
    extern uint switcher_width;
    extern uint switcher_height;
    extern uint switcher_button_height;
    extern ulong switcher_color;

    /**Run menu*/
    extern uint run_menu_width;
    extern uint run_menu_height;
    extern uint run_menu_button_height;
    extern ulong run_menu_color;

    /**General double click time*/
    extern ulong double_click_time;

    /**Background image path*/
    extern std::string background_path;

    std::string get_config_file_path();
};