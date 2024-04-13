#pragma once

#include "config.h"
#include "window_manager.h"

#include <memory>

#define SWITCHER              EshyWM::switcher

//#define DISPLAY               WindowManager::display
//#define ROOT                  DefaultRootWindow(WindowManager::display)
//#define DEFAULT_VISUAL(i)     DefaultVisual(WindowManager::display, i)

class EshyWMSwitcher;

namespace EshyWM
{
    bool initialize();
    void on_screen_resolution_changed(uint new_width, uint new_height);

    void window_created_notify(EshyWMWindow* window);
    void window_destroyed_notify(EshyWMWindow* window);

    extern std::shared_ptr<WindowManager> window_manager;
    extern std::shared_ptr<EshyWMSwitcher> switcher;

    extern bool b_terminate;
};