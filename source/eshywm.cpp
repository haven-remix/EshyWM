#include "eshywm.h"

#include <glog/logging.h>
#include <fstream>
#include <iostream>

static ::EshyWMConfig* eshywm_config;
static ::WindowManager* window_manager;

namespace EshyWM
{
::EshyWMConfig* get_eshywm_config()
{
    return eshywm_config;
}

::WindowManager* get_window_manager()
{
    return window_manager;
}


bool initialize()
{
    update_config();
    update_background();
    run_startup_commands();

    window_manager = WindowManager::Create();
    if(!window_manager)
    {
        LOG(ERROR) << "Failed to initialize window manager.";
        return false;
    }

    window_manager->Run();
    return true;
}

void update_config()
{
    if(!eshywm_config)
    {
        eshywm_config = new EshyWMConfig();
    }

    std::ifstream config_file(Internal::get_config_file_path());
    std::string line;
    while(std::getline(config_file, line))
    {
        if(line.find("background:") != std::string::npos)
        {
            std::size_t spos = line.find("\"");
            std::size_t npos = line.length() - 1;
            eshywm_config->background_path = line.substr(spos, npos);
        }
        else if(line.find("resize_step_size_width:") != std::string::npos)
        {
            std::size_t spos = line.find(" ");
            std::size_t npos = line.length() - 1;
            eshywm_config->resize_step_size_width = std::stoi(line.substr(spos, npos));
        }
        else if(line.find("resize_step_size_height:") != std::string::npos)
        {
            std::size_t spos = line.find(" ");
            std::size_t npos = line.length() - 1;
            eshywm_config->resize_step_size_height = std::stoi(line.substr(spos, npos));
        }
        else if(line.find("frame_window:") != std::string::npos)
        {
            std::size_t spos = line.find(" ");
            std::size_t npos = line.length() - 1;
            eshywm_config->frame_window = line.find("true") != std::string::npos ? true : false;
        }
        else if(line.find("window_frame_border_width:") != std::string::npos)
        {
            std::size_t spos = line.find(" ");
            std::size_t npos = line.length() - 1;
            eshywm_config->window_frame_border_width = std::stoi(line.substr(spos, npos));
        }
        else if(line.find("window_frame_border_color:") != std::string::npos)
        {
            std::size_t spos = line.find(" ");
            std::size_t npos = line.length() - 1;
            eshywm_config->window_frame_border_color = std::stoi(line.substr(spos, npos));
        }
        else if(line.find("window_background_color:") != std::string::npos)
        {
            std::size_t spos = line.find(" ");
            std::size_t npos = line.length() - 1;
            eshywm_config->window_background_color = std::stoi(line.substr(spos, npos));
        }
        else if(line.find("window_padding:") != std::string::npos)
        {
            std::size_t spos = line.find(" ");
            std::size_t npos = line.length() - 1;
            eshywm_config->window_padding = std::stoi(line.substr(spos, npos));
        }
    }
    config_file.close();
}

void run_startup_commands()
{
    std::ifstream config_file(Internal::get_config_file_path());
    std::string current_command;
    bool b_in_startup_section = false;
    while(std::getline(config_file, current_command))
    {   
        if(current_command.find("[startup-commands]") != std::string::npos)
        {
            b_in_startup_section = true;
            continue;
        }
        else if(current_command.find("[~startup-commands]") != std::string::npos)
        {
            b_in_startup_section = false;
            break;
        }

        if(b_in_startup_section)
        {
            system((current_command + " &").c_str());
        }
    }
    config_file.close();
}


void on_screen_resolution_changed()
{
    //Reset the background to match the screen size
    update_background();
}


void update_background(std::string _background_path)
{   
    std::string background_path = _background_path.empty() ? eshywm_config->background_path : _background_path;

    //Reset background
    system(("feh --bg-scale " + background_path).c_str());
}

namespace Internal
{
std::string get_config_file_path()
{
    return std::string(getenv("HOME")) + "/.config" + "/eshywm.conf";
}
}
}