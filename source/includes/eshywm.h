#pragma once

#include "config.h"
#include "window_manager.h"

#include <memory>

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

namespace EshyWM
{
    bool initialize();
    void on_screen_resolution_changed(uint new_width, uint new_height);
    void update_monitor_info();

    void window_created_notify(std::shared_ptr<EshyWMWindow> window);
    void window_destroyed_notify(std::shared_ptr<EshyWMWindow> window);

    extern std::vector<std::shared_ptr<s_monitor_info>> monitors;

    extern std::shared_ptr<EshyWMTaskbar> taskbar;
    extern std::shared_ptr<EshyWMSwitcher> switcher;
    extern std::shared_ptr<EshyWMContextMenu> context_menu;
    extern std::shared_ptr<EshyWMRunMenu> run_menu;

    extern bool b_terminate;
};