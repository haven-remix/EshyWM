
#include <chrono>

#define POWER_SUPPLY_STATUS_UNKNOWN_IDENTIFIER "Unknown"
#define POWER_SUPPLY_STATUS_CHARGING_IDENTIFIER "Charging"
#define POWER_SUPPLY_STATUS_DISCHARGING_IDENTIFIER "Discharging"
#define POWER_SUPPLY_STATUS_NOT_CHARGING_IDENTIFIER "Not Charging"
#define POWER_SUPPLY_STATUS_FULL_IDENTIFIER "Full"

enum EPowerSupplyStatus
{
    POWER_SUPPLY_STATUS_UNKNOWN,
    POWER_SUPPLY_STATUS_CHARGING,
    POWER_SUPPLY_STATUS_DISCHARGING,
    POWER_SUPPLY_STATUS_NOT_CHARGING,
    POWER_SUPPLY_STATUS_FULL
};

namespace System
{
    void begin_polling();
    void end_polling();
    void poll_system_info();

    extern EPowerSupplyStatus power_supply_status;
    extern int battery_percentage;
    extern std::chrono::_V2::system_clock::time_point current_time;
};