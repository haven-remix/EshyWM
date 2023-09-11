#pragma once

#include "config.h"
#include "window_manager.h"

#include <string>
#include <memory>
#include <iostream>

//#define TASKBAR               EshyWM::taskbar
#define SWITCHER              EshyWM::switcher
#define CONTEXT_MENU          EshyWM::context_menu
#define RUN_MENU              EshyWM::run_menu

#define DISPLAY               WindowManager::display
#define ROOT                  DefaultRootWindow(WindowManager::display)
#define DEFAULT_VISUAL(i)     DefaultVisual(WindowManager::display, i)

class EshyWMTaskbar;
class EshyWMSwitcher;
class EshyWMContextMenu;
class EshyWMRunMenu;
class System;

namespace EshyWM
{
    bool initialize();
    void on_screen_resolution_changed(uint new_width, uint new_height);

    extern std::shared_ptr<EshyWMTaskbar> taskbar;
    extern std::shared_ptr<EshyWMSwitcher> switcher;
    extern std::shared_ptr<EshyWMContextMenu> context_menu;
    extern std::shared_ptr<EshyWMRunMenu> run_menu;
    extern std::shared_ptr<System> system_info;

    extern bool b_terminate;
};