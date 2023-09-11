
#include <chrono>
#include <thread>

class System
{
public:

    System()
        : b_poll(false)
        , battery_percentage(0)
    {}

    void begin_polling();
    void end_polling();
    void poll_system_info();

    int battery_percentage;
    std::chrono::_V2::system_clock::time_point current_time;

private:


    std::thread poll_thread;
    bool b_poll;
};