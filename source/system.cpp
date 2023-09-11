
#include "system.h"
#include "window_manager.h"
#include "taskbar.h"
#include "eshywm.h"

#include <iostream>
#include <unistd.h>
#include <memory>
#include <array>

using namespace std::chrono_literals;

static std::string exec(const char* command)
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command, "r"), pclose);

    if(!pipe)
        throw std::runtime_error("popen() failed!");

    while (fgets(buffer.data(), buffer.size(), pipe.get()))
        result += buffer.data();

    return result;
}

void System::begin_polling()
{
    b_poll = true;
    poll_thread = std::thread(&System::poll_system_info, this);
}

void System::end_polling()
{
    b_poll = false;
}

void System::poll_system_info()
{
    while(b_poll)
    {
        battery_percentage = std::stoi(exec("cat /sys/class/power_supply/BAT0/capacity"));
        current_time = std::chrono::system_clock::now();
        std::this_thread::sleep_for(500ms);
    }
}