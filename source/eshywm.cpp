#include "eshywm.h"
#include "system.h"
#include "window.h"
#include "window_manager.h"
#include "config.h"
#include "context_menu.h"
#include "taskbar.h"
#include "switcher.h"
#include "run_menu.h"
#include "util.h"

#include <X11/extensions/Xrandr.h>

std::vector<std::shared_ptr<s_monitor_info>> EshyWM::monitors;

std::shared_ptr<EshyWMTaskbar> EshyWM::taskbar;
std::shared_ptr<EshyWMSwitcher> EshyWM::switcher;
std::shared_ptr<EshyWMContextMenu> EshyWM::context_menu;
std::shared_ptr<EshyWMRunMenu> EshyWM::run_menu;

bool EshyWM::b_terminate = false;

void EshyWM::update_monitor_info()
{
    int n_monitors;
    XRRMonitorInfo* found_monitors = XRRGetMonitors(DISPLAY, ROOT, false, &n_monitors);

    monitors.clear();

    for(int i = 0; i < n_monitors; i++)
    {
        std::shared_ptr<s_monitor_info> monitor_info = std::make_shared<s_monitor_info>((bool)found_monitors[i].primary, found_monitors[i].x, found_monitors[i].y, (uint)found_monitors[i].width, (uint)found_monitors[i].height);
        monitors.push_back(monitor_info);
    }

    XRRFreeMonitors(found_monitors);
}

bool EshyWM::initialize()
{
    EshyWMConfig::update_config();
    EshyWMConfig::update_data();
    for(const std::string command : EshyWMConfig::startup_commands)
        system((command + "&").c_str());
    
    system(("feh --bg-scale " + std::string(EshyWMConfig::background_path)).c_str());

    WindowManager window_manager;
    window_manager.initialize();

    imlib_context_set_display(DISPLAY);
    imlib_context_set_visual(DefaultVisual(DISPLAY, DefaultScreen(DISPLAY)));
    imlib_context_set_colormap(DefaultColormap(DISPLAY, DefaultScreen(DISPLAY)));

    context_menu = std::make_shared<EshyWMContextMenu>(Rect{0, 0, EshyWMConfig::context_menu_width, 150}, EshyWMConfig::context_menu_color);
    switcher = std::make_shared<EshyWMSwitcher>(Rect{CENTER_W(monitors[0], 50), CENTER_H(monitors[0], EshyWMConfig::switcher_button_height), 50, 50}, EshyWMConfig::switcher_color);
    run_menu = std::make_shared<EshyWMRunMenu>(Rect{CENTER_W(monitors[0], EshyWMConfig::run_menu_width), CENTER_H(monitors[0], EshyWMConfig::run_menu_height), EshyWMConfig::run_menu_width, EshyWMConfig::run_menu_height}, EshyWMConfig::run_menu_color);
    
    //Create a taskbar for each monitor. This is so only monitors with fullscreened windows do not have a taskbar. 
    for(const std::shared_ptr<s_monitor_info> monitor : monitors)
    {
        monitor->taskbar = std::make_shared<EshyWMTaskbar>(Rect{monitor->x, (int)(monitor->height - EshyWMConfig::taskbar_height), monitor->width, EshyWMConfig::taskbar_height}, EshyWMConfig::taskbar_color);
        monitor->taskbar->show();
    }

    window_manager.handle_preexisting_windows();
    System::begin_polling();

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

        for(std::shared_ptr<s_monitor_info> monitor : monitors)
            monitor->taskbar->display_system_info();

        window_manager.handle_events();
    }

    System::end_polling();
    return true;
}

void EshyWM::on_screen_resolution_changed(uint new_width, uint new_height)
{
    //TASKBAR->update_taskbar_size(new_width, new_height);

    //Reset the background to match the screen size
    system(("feh --bg-scale " + std::string(EshyWMConfig::background_path)).c_str());
}


void EshyWM::window_created_notify(std::shared_ptr<EshyWMWindow> window)
{
    if(!window)
        return;

    for(std::shared_ptr<s_monitor_info> monitor : monitors)
        if(monitor->taskbar)
            monitor->taskbar->add_button(window, window->get_window_icon());

    if(switcher)
        switcher->add_window_option(window, window->get_window_icon());
}

void EshyWM::window_destroyed_notify(std::shared_ptr<EshyWMWindow> window)
{
    if(!window)
        return;

    for(std::shared_ptr<s_monitor_info> monitor : EshyWM::monitors)
        if(monitor->taskbar)
            monitor->taskbar->remove_button(window);

    if(switcher)
        switcher->remove_window_option(window);
}