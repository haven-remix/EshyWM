#pragma once

#include "config.h"
#include "window_manager.h"

#include <string>
#include <memory>
#include <iostream>

#define TASKBAR               EshyWM::Internal::taskbar
#define SWITCHER              EshyWM::Internal::switcher
#define CONTEXT_MENU          EshyWM::Internal::context_menu
#define RUN_MENU              EshyWM::Internal::run_menu

#define DISPLAY_WIDTH         WindowManager::Internal::display_width
#define DISPLAY_HEIGHT        WindowManager::Internal::display_height

#define DISPLAY               WindowManager::Internal::display
#define ROOT                  DefaultRootWindow(WindowManager::Internal::display)
#define IS_TILING_MODE()      WindowManager::Internal::manager_data->b_tiling_mode

#define ensure(s)             if(!(s)) abort();

class EshyWMTaskbar;
class EshyWMSwitcher;
class EshyWMContextMenu;
class EshyWMRunMenu;

namespace EshyWM
{
    bool initialize();
    void on_screen_resolution_changed(uint new_width, uint new_height);

namespace Internal
{
    extern std::shared_ptr<EshyWMTaskbar> taskbar;
    extern std::shared_ptr<EshyWMSwitcher> switcher;
    extern std::shared_ptr<EshyWMContextMenu> context_menu;
    extern std::shared_ptr<EshyWMRunMenu> run_menu;
};
};