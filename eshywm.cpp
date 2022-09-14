#include "eshywm.h"
#include "window_manager.h"

#include <glog/logging.h>
#include <fstream>

namespace EshyWM
{
bool initialize()
{
    run_startup_commands();

    std::unique_ptr<WindowManager> window_manager(WindowManager::Create());
    if(!window_manager)
    {
        LOG(ERROR) << "Failed to initialize window manager.";
        return false;
    }

    window_manager->Run();
    return true;
}

void run_startup_commands()
{
    std::ifstream config_file(std::string(getenv("HOME")) + "/.eshywmconfig");
    std::string current_command;
    while(std::getline(config_file, current_command))
    {
        system((current_command + " &").c_str());
    }
}

void on_screen_resolution_changed()
{
    //@fixme: used the configured background
    //Reset the background to match the screen size
    system("feh --bg-scale ~/bg.jpg");
}
}