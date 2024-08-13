#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace EshyWMConfig
{
    /**Window frame*/
    extern uint window_frame_border_width;
    extern ulong window_frame_border_color;
    extern ulong window_background_color;
    extern ulong close_button_color;
    extern std::string minimize_button_image_path;
    extern std::string maximize_button_image_path;
    extern std::string close_button_image_path;

    extern float window_opacity_step;
    extern int window_x_movement_step;
    extern int window_y_movement_step;
    extern int window_width_resize_step;
    extern int window_height_resize_step;

    /**Titlebar*/
    extern uint titlebar_height;
    extern uint titlebar_button_size;
    extern uint titlebar_button_padding;
    extern ulong titlebar_button_normal_color;
    extern ulong titlebar_button_hovered_color;
    extern ulong titlebar_button_pressed_color;
    extern ulong titlebar_title_color;
    extern bool titlebar;

    /**Switcher*/
    extern uint switcher_button_height;
    extern uint switcher_button_padding;
    extern ulong switcher_button_color;
    extern ulong switcher_button_border_color;
    extern ulong switcher_color;

    /**General double click time*/
    extern ulong double_click_time;

    /**Background image path*/
    extern std::string background_path;
    extern std::string default_application_image_path;

    extern std::vector<std::string> startup_commands;
    extern std::unordered_map<std::string, std::string> window_close_data;

    struct KeyBinding
    {
        int key;
        std::string command;
    };

    extern std::vector<KeyBinding> key_bindings;

    void update_config();
    void update_data();
    void update_data_file();

    void add_window_close_state(std::string window, std::string new_state);
};