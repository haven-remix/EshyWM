
#include "system.h"
#include "window_manager.h"
#include "taskbar.h"
#include "eshywm.h"

#include <iostream>
#include <unistd.h>
#include <memory>
#include <array>
#include <string.h>
#include <thread>

using namespace std::chrono_literals;

EPowerSupplyStatus System::power_supply_status = POWER_SUPPLY_STATUS_UNKNOWN;
int System::battery_percentage = 100;
std::chrono::_V2::system_clock::time_point System::current_time;

std::thread poll_thread;
bool b_poll = false;

static char* exec(const char* command)
{
    char buffer[128];
    char* result = (char*)calloc(sizeof(buffer), sizeof(char));
    FILE* pipe(popen(command, "r"));

    if(!pipe)
        throw std::runtime_error("popen() failed!");

    while (fgets(buffer, sizeof(buffer), pipe))
        strcat(result, buffer);

    pclose(pipe);

    return result;
}

void System::begin_polling()
{
    b_poll = true;
    poll_thread = std::thread(&System::poll_system_info);
}

void System::end_polling()
{
    b_poll = false;
}

void System::poll_system_info()
{
    while(b_poll)
    {
        const char* charging_status = exec("cat /sys/class/power_supply/BAT0/status");
        if(charging_status == POWER_SUPPLY_STATUS_NOT_CHARGING_IDENTIFIER)
            power_supply_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
        else if(charging_status == POWER_SUPPLY_STATUS_CHARGING_IDENTIFIER)
            power_supply_status = POWER_SUPPLY_STATUS_CHARGING;
        else if(charging_status == POWER_SUPPLY_STATUS_DISCHARGING_IDENTIFIER)
            power_supply_status = POWER_SUPPLY_STATUS_DISCHARGING;
        else if(charging_status == POWER_SUPPLY_STATUS_FULL_IDENTIFIER)
            power_supply_status = POWER_SUPPLY_STATUS_FULL;
        else
            power_supply_status = POWER_SUPPLY_STATUS_UNKNOWN;

        battery_percentage = std::stoi(exec("cat /sys/class/power_supply/BAT0/capacity"));
        current_time = std::chrono::system_clock::now();
        std::this_thread::sleep_for(500ms);
    }
}