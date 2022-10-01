#pragma once

#include <string>
#include <memory>

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

    void on_screen_resolution_changed();

    void update_background(std::string background_path = std::string());

    /**Getters*/
    static std::shared_ptr<class EshyWMConfig> get_current_config() {return EshyWM::Get().current_config;}
    static std::shared_ptr<class WindowManager> get_window_manager() {return EshyWM::Get().window_manager;}

    EshyWM(const EshyWM&) = delete;

private:

    EshyWM() {};

    std::shared_ptr<class EshyWMConfig> current_config;
    std::shared_ptr<class WindowManager> window_manager;
};