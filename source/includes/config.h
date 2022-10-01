#pragma once

class EshyWMConfig
{
public:

    /**From config file*/

    int window_padding;

    int resize_step_size_width;
    int resize_step_size_height;

    bool frame_window;
    int window_frame_border_width;
    long window_frame_border_color;
    long window_background_color;

    std::string background_path;

    /**Runtime*/

    bool b_floating_mode;

    void initialize_values()
    {
        window_padding = 0;

        resize_step_size_width = 10;
        resize_step_size_height = 10;

        frame_window = true;
        window_frame_border_width = 0;
        window_frame_border_color = 0;
        window_background_color = 0;

        background_path = "";

        b_floating_mode = false;
    }

    std::string get_config_file_path()
    {
        return std::string(getenv("HOME")) + "/.config" + "/eshywm.conf";
    }
};