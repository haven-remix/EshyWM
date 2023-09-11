#include "eshywm.h"
#include "system.h"
#include "window_manager.h"
#include "config.h"
#include "context_menu.h"
#include "taskbar.h"
#include "switcher.h"
#include "run_menu.h"
#include "util.h"

#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>

std::shared_ptr<EshyWMTaskbar> EshyWM::taskbar;
std::shared_ptr<EshyWMSwitcher> EshyWM::switcher;
std::shared_ptr<EshyWMContextMenu> EshyWM::context_menu;
std::shared_ptr<EshyWMRunMenu> EshyWM::run_menu;
std::shared_ptr<System> EshyWM::system_info;

bool EshyWM::b_terminate = false;

bool EshyWM::initialize()
{
    EshyWMConfig::update_config();
    for(const std::string command : EshyWMConfig::startup_commands)
        system((command + "&").c_str());
    
    system(("feh --bg-scale " + std::string(EshyWMConfig::background_path)).c_str());

    WindowManager::initialize();

    imlib_context_set_display(DISPLAY);
    imlib_context_set_visual(DefaultVisual(DISPLAY, DefaultScreen(DISPLAY)));
    imlib_context_set_colormap(DefaultColormap(DISPLAY, DefaultScreen(DISPLAY)));

    context_menu = std::make_shared<EshyWMContextMenu>(rect{0, 0, EshyWMConfig::context_menu_width, 150}, EshyWMConfig::context_menu_color);
    switcher = std::make_shared<EshyWMSwitcher>(rect{CENTER_W(WindowManager::monitors[0], 50), CENTER_H(WindowManager::monitors[0], EshyWMConfig::switcher_button_height), 50, 50}, EshyWMConfig::switcher_color);
    run_menu = std::make_shared<EshyWMRunMenu>(rect{CENTER_W(WindowManager::monitors[0], EshyWMConfig::run_menu_width), CENTER_H(WindowManager::monitors[0], EshyWMConfig::run_menu_height), EshyWMConfig::run_menu_width, EshyWMConfig::run_menu_height}, EshyWMConfig::run_menu_color);

    system_info = std::make_shared<System>();
    
    //Create a taskbar for each monitor. This is so only monitors will fullscreened windows do not have a taskbar. 
    for(const std::shared_ptr<s_monitor_info> monitor : WindowManager::monitors)
    {
        monitor->taskbar = std::make_shared<EshyWMTaskbar>(rect{monitor->x, (int)(monitor->height - EshyWMConfig::taskbar_height), monitor->width, EshyWMConfig::taskbar_height}, EshyWMConfig::taskbar_color);
        monitor->taskbar->show();
    }

    WindowManager::handle_preexisting_windows();
    system_info->begin_polling();

    const int x11_file_descriptor = ConnectionNumber(DISPLAY);
    fd_set in_file_descriptor_set;
    struct timeval time_value;

    while(!b_terminate)
    {
        FD_ZERO(&in_file_descriptor_set);
        FD_SET(x11_file_descriptor, &in_file_descriptor_set);

        time_value.tv_sec = 0;
        time_value.tv_usec = 500000;

        select(x11_file_descriptor + 1, &in_file_descriptor_set, 0, 0, &time_value);

        for(std::shared_ptr<s_monitor_info> monitor : WindowManager::monitors)
            monitor->taskbar->display_system_info();

        WindowManager::handle_events();
    }

    system_info->end_polling();
    return true;
}

void EshyWM::on_screen_resolution_changed(uint new_width, uint new_height)
{
    //TASKBAR->update_taskbar_size(new_width, new_height);

    //Reset the background to match the screen size
    system(("feh --bg-scale " + std::string(EshyWMConfig::background_path)).c_str());
}