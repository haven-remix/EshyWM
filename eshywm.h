#pragma once

#include <string>
#include <memory>

#include "config.h"
#include "window_manager.h"

namespace EshyWM
{
::EshyWMConfig* get_eshywm_config();
::WindowManager* get_window_manager();

bool initialize();
void update_config();
void run_startup_commands();

void on_screen_resolution_changed();

void update_background(std::string background_path = std::string());

namespace Internal
{
std::string get_config_file_path();
}
}