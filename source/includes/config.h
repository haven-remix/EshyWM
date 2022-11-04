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

    uint window_frame_border_width;
    unsigned long window_frame_border_color;
    unsigned long window_background_color;

    int titlebar_height;
    uint titlebar_button_size;
    uint titlebar_button_padding;
    unsigned long titlebar_button_normal_color;
    unsigned long titlebar_button_hovered_color;
    unsigned long titlebar_button_pressed_color;
    unsigned long titlebar_title_color;

    uint context_menu_width;
    unsigned long context_menu_color;
    unsigned long context_menu_secondary_color;

    uint taskbar_height;
    unsigned long taskbar_color;

    uint switcher_width;
    uint switcher_height;
    uint switcher_button_height;
    unsigned long switcher_color;

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

        context_menu_width = 250;
        context_menu_color = 0xeeeeee;
        context_menu_secondary_color = 0x222222;

        taskbar_height = 30;
        taskbar_color = 0x283140;

        switcher_width = 300;
        switcher_height = 500;
        switcher_button_height = 30;
        switcher_color = 0xaeb3bd;

        double_click_time = 500;

        background_path = "";

        b_floating_mode = false;
    }

    std::string get_config_file_path()
    {
        return std::string(getenv("HOME")) + "/.config" + "/eshywm.conf";
    }
};