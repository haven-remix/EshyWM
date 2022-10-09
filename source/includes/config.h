#pragma once

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
    unsigned long titlebar_button_color;

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

        titlebar_height = 20;
        titlebar_button_size = 18;
        titlebar_button_padding = 4;
        titlebar_button_color = 0x47436b;

        background_path = "";

        b_floating_mode = false;
    }

    std::string get_config_file_path()
    {
        return std::string(getenv("HOME")) + "/.config" + "/eshywm.conf";
    }
};