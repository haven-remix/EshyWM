#include "eshywm.h"
#include "window_manager.h"
#include "config.h"

#include <glog/logging.h>
#include <fstream>
#include <iostream>

bool EshyWM::initialize()
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

void EshyWM::update_config()
{
    //Make a config class if there isn't one already
    if(!get_current_config())
    {
        current_config = std::make_shared<EshyWMConfig>();
    }

    //Initialize variable (in case some are not in the file, we will have default values)
    get_current_config()->initialize_values();

    std::ifstream config_file(get_current_config()->get_config_file_path());
    std::string line;

    while(std::getline(config_file, line))
    {
        if(line.find("background:") != std::string::npos)
        {
            std::size_t spos = line.find("\"");
            std::size_t npos = line.length() - 1;
            get_current_config()->background_path = line.substr(spos, npos);
        }
        else if(line.find("resize_step_size_width:") != std::string::npos)
        {
            std::size_t spos = line.find(" ");
            std::size_t npos = line.length() - 1;
            get_current_config()->resize_step_size_width = std::stoi(line.substr(spos, npos));
        }
        else if(line.find("resize_step_size_height:") != std::string::npos)
        {
            std::size_t spos = line.find(" ");
            std::size_t npos = line.length() - 1;
            get_current_config()->resize_step_size_height = std::stoi(line.substr(spos, npos));
        }
        else if(line.find("window_frame_border_width:") != std::string::npos)
        {
            std::size_t spos = line.find(" ");
            std::size_t npos = line.length() - 1;
            get_current_config()->window_frame_border_width = std::stoi(line.substr(spos, npos));
        }
        else if(line.find("window_frame_border_color:") != std::string::npos)
        {
            std::size_t spos = line.find(" ");
            std::size_t npos = line.length() - 1;
            get_current_config()->window_frame_border_color = std::stoi(line.substr(spos, npos));
        }
        else if(line.find("window_background_color:") != std::string::npos)
        {
            std::size_t spos = line.find(" ");
            std::size_t npos = line.length() - 1;
            get_current_config()->window_background_color = std::stoi(line.substr(spos, npos));
        }
        else if(line.find("window_padding:") != std::string::npos)
        {
            std::size_t spos = line.find(" ");
            std::size_t npos = line.length() - 1;
            get_current_config()->window_padding = std::stoi(line.substr(spos, npos));
        }
    }

    config_file.close();
}

void EshyWM::run_startup_commands()
{
    std::ifstream config_file(get_current_config()->get_config_file_path());
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


void EshyWM::on_screen_resolution_changed(uint new_width, uint new_height)
{
    //Reset the background to match the screen size
    update_background();
}


void EshyWM::update_background(std::string _background_path)
{   
    std::string background_path = _background_path.empty() ? get_current_config()->background_path : _background_path;

    //Reset background
    system(("feh --bg-scale " + background_path).c_str());
}