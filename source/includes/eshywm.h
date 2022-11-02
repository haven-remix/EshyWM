#pragma once

#include "config.h"
#include "window_manager.h"

#include <string>
#include <memory>
#include <iostream>

#define WINDOW_MANAGER        EshyWM::get_window_manager()
#define CONFIG                EshyWM::get_current_config()
#define DISPLAY               EshyWM::get_window_manager()->get_display()
#define ROOT                  EshyWM::get_window_manager()->get_root()
#define IS_TILING_MODE()      EshyWM::get_window_manager()->get_manager_data()->b_tiling_mode

#define PRIMARY_MOD_KEY       Mod1Mask | ShiftMask
#define SECONDARY_MOD_KEY     Mod4Mask

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

    void update_background(std::string background_path = std::string());

    /**Getters*/
    static std::shared_ptr<class EshyWMConfig> get_current_config() {return EshyWM::Get().current_config;}
    static std::shared_ptr<class WindowManager> get_window_manager() {return EshyWM::Get().window_manager;}

    EshyWM(const EshyWM&) = delete;

private:

    EshyWM() {}

    std::shared_ptr<class EshyWMConfig> current_config;
    std::shared_ptr<class WindowManager> window_manager;
};