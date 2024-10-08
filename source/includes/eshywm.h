#pragma once

#include "config.h"
#include "window_manager.h"

#include <memory>

#define SWITCHER              EshyWM::switcher

class EshyWMSwitcher;

namespace EshyWM
{
    bool initialize();
    void on_screen_resolution_changed(uint new_width, uint new_height);

    void window_created_notify(std::shared_ptr<EshyWMWindow> window);
    void window_destroyed_notify(std::shared_ptr<EshyWMWindow> window);

    extern std::shared_ptr<WindowManager> window_manager;
    extern std::shared_ptr<EshyWMSwitcher> switcher;

    extern bool b_terminate;
};