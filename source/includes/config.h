#pragma once

extern "C" {
#include <X11/Xlib.h>
}

#include <string>

class EshyWMConfig
{
public:

    /**From config file*/

    int window_padding;

    int resize_step_size_width;
    int resize_step_size_height;

    unsigned int window_frame_border_width;
    unsigned long window_frame_border_color;
    unsigned long window_background_color;

    int titlebar_height;
    unsigned int titlebar_button_size;
    unsigned int titlebar_button_padding;
    unsigned long titlebar_button_normal_color;
    unsigned long titlebar_button_hovered_color;
    unsigned long titlebar_button_pressed_color;
    unsigned long titlebar_title_color;

    unsigned int taskbar_height;
    unsigned long taskbar_color;

    Time double_click_time;

    std::string background_path;

    /**Runtime*/

    bool b_floating_mode;

    void initialize_values()
    {
        window_padding = 0;

        resize_step_size_width = 10;
        resize_step_size_height = 10;

        window_frame_border_width = 2;
        window_frame_border_color = 0x3d3a5c;
        window_background_color = 0x15141f;

        titlebar_height = 22;
        titlebar_button_size = 18;
        titlebar_button_padding = 4;
        titlebar_button_normal_color = 0x47436b;
        titlebar_button_hovered_color = 0x47436b;
        titlebar_title_color = 0xededed;

        taskbar_height = 30;
        taskbar_color = 0x283140;

        double_click_time = 500;

        background_path = "";

        b_floating_mode = false;
    }

    std::string get_config_file_path()
    {
        return std::string(getenv("HOME")) + "/.config" + "/eshywm.conf";
    }
};