#pragma once

#include "config.h"
#include "window_manager.h"

#include <string>
#include <memory>
#include <iostream>

#define WINDOW_MANAGER        EshyWM::get_window_manager()
#define CONFIG                EshyWM::get_current_config()
#define TASKBAR               EshyWM::get_taskbar()
#define SWITCHER              EshyWM::get_switcher()
#define CONTEXT_MENU          EshyWM::get_context_menu()

#define DISPLAY               EshyWM::get_window_manager()->get_display()
#define ROOT                  EshyWM::get_window_manager()->get_root()
#define IS_TILING_MODE()      EshyWM::get_window_manager()->get_manager_data()->b_tiling_mode

struct s_window_desktop_entry_data
{
    std::string name;
    std::string comment;
    std::string exec_path;
    std::string icon;
};

typedef std::unordered_map<std::string, s_window_desktop_entry_data> window_desktop_entry_data_t;

class EshyWM
{
public:

    static EshyWM& Get()
    {
        static EshyWM instance;
        return instance;
    }

    bool initialize();
    void update_config();
    void run_startup_commands();

    void on_screen_resolution_changed(uint new_width, uint new_height);

    /**Getters*/
    static std::shared_ptr<class EshyWMConfig> get_current_config() {return EshyWM::Get().current_config;}
    static std::shared_ptr<class WindowManager> get_window_manager() {return EshyWM::Get().window_manager;}
    static std::shared_ptr<class EshyWMTaskbar> get_taskbar() {return EshyWM::Get().taskbar;}
    static std::shared_ptr<class EshyWMSwitcher> get_switcher() {return EshyWM::Get().switcher;}
    static std::shared_ptr<class EshyWMContextMenu> get_context_menu() {return EshyWM::Get().context_menu;}
    window_desktop_entry_data_t get_window_desktop_entry_data() {return window_desktop_entry_data;}

    EshyWM(const EshyWM&) = delete;

private:

    EshyWM() {}

    std::shared_ptr<class EshyWMConfig> current_config;
    std::shared_ptr<class WindowManager> window_manager;
    std::shared_ptr<class EshyWMTaskbar> taskbar;
    std::shared_ptr<class EshyWMSwitcher> switcher;
    std::shared_ptr<class EshyWMContextMenu> context_menu;

    window_desktop_entry_data_t window_desktop_entry_data;

    void update_window_desktop_entries();
    void update_background(std::string background_path = std::string());
};