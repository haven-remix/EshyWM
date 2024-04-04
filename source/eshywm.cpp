#include "eshywm.h"
#include "system.h"
#include "window.h"
#include "window_manager.h"
#include "config.h"
#include "switcher.h"
#include "util.h"

#include <X11/extensions/Xrandr.h>

std::shared_ptr<WindowManager> EshyWM::window_manager;
std::shared_ptr<EshyWMSwitcher> EshyWM::switcher;

bool EshyWM::b_terminate = false;

bool EshyWM::initialize()
{
    EshyWMConfig::update_config();
    EshyWMConfig::update_data();
    
    system(("feh --bg-scale " + std::string(EshyWMConfig::background_path)).c_str());

    window_manager = std::make_shared<WindowManager>();
    window_manager->initialize();

    imlib_context_set_display(DISPLAY);
    imlib_context_set_visual(DefaultVisual(DISPLAY, DefaultScreen(DISPLAY)));
    imlib_context_set_colormap(DefaultColormap(DISPLAY, DefaultScreen(DISPLAY)));

    switcher = std::make_shared<EshyWMSwitcher>(Rect{center_x(window_manager->outputs[0], 50), center_y(window_manager->outputs[0], EshyWMConfig::switcher_button_height), 50, 50}, EshyWMConfig::switcher_color);

    for(const std::string command : EshyWMConfig::startup_commands)
    {
        system((command + "&").c_str());
    }

    window_manager->handle_preexisting_windows();
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

        window_manager->handle_events();
    }

    System::end_polling();
    return true;
}

void EshyWM::on_screen_resolution_changed(uint new_width, uint new_height)
{
    //Reset the background to match the screen size
    system(("feh --bg-scale " + std::string(EshyWMConfig::background_path)).c_str());
}


void EshyWM::window_created_notify(EshyWMWindow* window)
{
    if(!window)
        return;

    if(switcher)
        switcher->add_window_option(window, window->get_window_icon());
}

void EshyWM::window_destroyed_notify(EshyWMWindow* window)
{
    if(!window)
        return;

    if(switcher)
        switcher->remove_window_option(window);
}