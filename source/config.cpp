#include "config.h"

#include <fstream>
#include <iostream>
#include <cstring>

//Window frame
uint EshyWMConfig::window_frame_border_width = 10;
ulong EshyWMConfig::window_frame_border_color = 0x3d3a5c;
ulong EshyWMConfig::window_background_color = 0x15141f;
ulong EshyWMConfig::close_button_color = 0xdb5353;
std::string EshyWMConfig::minimize_button_image_path = "";
std::string EshyWMConfig::maximize_button_image_path = "";
std::string EshyWMConfig::close_button_image_path = "";
float EshyWMConfig::window_opacity_step = 0.1;
int EshyWMConfig::window_x_movement_step = 50;
int EshyWMConfig::window_y_movement_step = 50;
int EshyWMConfig::window_width_resize_step = 50;
int EshyWMConfig::window_height_resize_step = 50;
//Titlebar
uint EshyWMConfig::titlebar_height = 26;
uint EshyWMConfig::titlebar_button_size = 26;
uint EshyWMConfig::titlebar_button_padding = 4;
ulong EshyWMConfig::titlebar_button_normal_color = 0x15141f;
ulong EshyWMConfig::titlebar_button_hovered_color = 0x423d66;
ulong EshyWMConfig::titlebar_button_pressed_color = 0x0d0c14;
ulong EshyWMConfig::titlebar_title_color = 0xededed;
//Switcher
uint EshyWMConfig::switcher_button_height = 140;
uint EshyWMConfig::switcher_button_padding = 20;
ulong EshyWMConfig::switcher_button_color = 0x888888;
ulong EshyWMConfig::switcher_button_border_color = 0x444444;
ulong EshyWMConfig::switcher_color = 0xaeb3bd;
//General click time
ulong EshyWMConfig::double_click_time = 500;
//Background image path
std::string EshyWMConfig::background_path = "";
std::string EshyWMConfig::default_application_image_path = "";

std::vector<EshyWMConfig::KeyBinding> EshyWMConfig::key_bindings;

std::vector<std::string> EshyWMConfig::startup_commands;
std::unordered_map<std::string, std::string> EshyWMConfig::window_close_data;

enum VarType
{
    VT_INT,
    VT_UINT,
    VT_UINT_HEX,
    VT_ULONG,
    VT_ULONG_HEX,
    VT_FLOAT,
    VT_STRING
};

struct key_value_pair
{
    std::string key;
    std::string value;
};

const std::string CONFIG_FILE_PATH = std::string(getenv("HOME")) + "/.config/eshywm/eshywm.conf";
const std::string DATA_FILE_PATH = std::string(getenv("HOME")) + "/.eshywm";

static void parse_config_option(std::string line, VarType type, void* config_var, std::string option_name)
{
    if(line.find(option_name) == std::string::npos)
        return;
    
    const size_t pos = line.find(': ');
    const key_value_pair kvp = {line.substr(0, pos).c_str(), line.substr(pos + 1, line.length() - pos).c_str()};

    switch(type)
    {
    case VarType::VT_INT:
        *((int*)config_var) = std::stoi(kvp.value);
        break;
    case VarType::VT_UINT:
        *((uint*)config_var) = std::stoi(kvp.value);
        break;
    case VarType::VT_UINT_HEX:
        *((uint*)config_var) = std::stoi(kvp.value, 0, 16);
        break;
    case VarType::VT_ULONG:
        *((ulong*)config_var) = std::stoul(kvp.value);
        break;
    case VarType::VT_ULONG_HEX:
        *((ulong*)config_var) = std::stoul(kvp.value, 0, 16);
        break;
    case VarType::VT_FLOAT:
        *((float*)config_var) = std::stoi(kvp.value);
        break;
    case VarType::VT_STRING:
        *(std::string*)config_var = kvp.value;
        break;
    };
}

void EshyWMConfig::update_config()
{
    std::ifstream config_file(CONFIG_FILE_PATH);
    std::string line;

    std::string startup_command;

    KeyBinding key_binding;

    while(std::getline(config_file, line))
    {
        const std::size_t comment_start_pos = line.find("#");
        if(comment_start_pos != std::string::npos)
        {
            line = line.substr(0, comment_start_pos);
        }

        parse_config_option(line, VT_STRING, &startup_command, "startup_command");

        if(!startup_command.empty())
        {
            startup_commands.push_back(startup_command);
            startup_command = "";
            continue;
        }

        parse_config_option(line, VT_INT, &key_binding.key, "keybind_key");
        parse_config_option(line, VT_STRING, &key_binding.command, "keybind_command");

        if(key_binding.key != 0 && key_binding.command != "")
        {
            key_bindings.emplace_back(key_binding.key, key_binding.command);
            key_binding.key = 0;
            key_binding.command = "";
            continue;
        }

        parse_config_option(line, VT_STRING, &background_path, "background:");
        parse_config_option(line, VT_STRING, &default_application_image_path, "default_application_image_path:");

        parse_config_option(line, VT_UINT, &window_frame_border_width, "window_frame_border_width");
        parse_config_option(line, VT_ULONG_HEX, &window_frame_border_color, "window_frame_border_color");
        parse_config_option(line, VT_ULONG_HEX, &window_background_color, "window_background_color");
        parse_config_option(line, VT_ULONG_HEX, &close_button_color, "close_button_color");
        parse_config_option(line, VT_STRING, &minimize_button_image_path, "minimize_button_image_path");
        parse_config_option(line, VT_STRING, &maximize_button_image_path, "maximize_button_image_path");
        parse_config_option(line, VT_STRING, &close_button_image_path, "close_button_image_path");

        parse_config_option(line, VT_FLOAT, &window_opacity_step, "window_opacity_step");
        parse_config_option(line, VT_INT, &window_x_movement_step, "window_x_movement_step");
        parse_config_option(line, VT_INT, &window_y_movement_step, "window_y_movement_step");
        parse_config_option(line, VT_INT, &window_width_resize_step, "window_width_resize_step");
        parse_config_option(line, VT_INT, &window_height_resize_step, "window_height_resize_step");

        parse_config_option(line, VT_UINT, &titlebar_height, "titlebar_height");
        parse_config_option(line, VT_UINT, &titlebar_button_size, "titlebar_button_size");
        parse_config_option(line, VT_UINT, &titlebar_button_padding, "titlebar_button_padding");
        parse_config_option(line, VT_ULONG_HEX, &titlebar_button_normal_color, "titlebar_button_normal_color");
        parse_config_option(line, VT_ULONG_HEX, &titlebar_button_hovered_color, "titlebar_button_hovered_color");
        parse_config_option(line, VT_ULONG_HEX, &titlebar_button_pressed_color, "titlebar_button_pressed_color");
        parse_config_option(line, VT_ULONG_HEX, &titlebar_title_color, "titlebar_title_color");

        parse_config_option(line, VT_ULONG, &switcher_button_height, "switcher_button_height");
        parse_config_option(line, VT_ULONG, &switcher_button_padding, "switcher_button_padding");
        parse_config_option(line, VT_ULONG_HEX, &switcher_button_color, "switcher_button_color");
        parse_config_option(line, VT_ULONG_HEX, &switcher_button_border_color, "switcher_button_border_color");
        parse_config_option(line, VT_ULONG_HEX, &switcher_color, "switcher_color");

        parse_config_option(line, VT_ULONG, &double_click_time, "double_click_time");
    }

    config_file.close();
}

void EshyWMConfig::update_data()
{
    std::ifstream config_file(DATA_FILE_PATH);
    std::string line;

    std::string window_data;

    while(std::getline(config_file, line))
    {
        parse_config_option(line, VT_STRING, &window_data, "window_close_data:");
        if(window_data != "")
        {
            const size_t pos = window_data.find(',');
            const key_value_pair kvp = {window_data.substr(0, pos).c_str(), window_data.substr(pos + 1, window_data.length() - pos).c_str()};
            window_close_data.emplace(kvp.key, kvp.value);
            window_data = "";
        }
    }

    config_file.close();
}

void EshyWMConfig::update_data_file()
{
    std::fstream config_file(DATA_FILE_PATH, std::ios_base::out);

    for(auto [a, b] : window_close_data)
        config_file << "window_close_data: " << a << "," << b << "\n";

    config_file.close();
}

void EshyWMConfig::add_window_close_state(std::string window, std::string new_state)
{
    window_close_data[window] = new_state;
    update_data_file();
}