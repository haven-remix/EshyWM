#pragma once

#include <Imlib2.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "util.h"

struct s_monitor_info
{
    s_monitor_info()
        : b_primary(false)
        , x(0)
        , y(0)
        , width(0)
        , height(0)
        , taskbar(nullptr)
    {}

    s_monitor_info(bool _b_primary, int _x, int _y, uint _width, uint _height)
        : b_primary(_b_primary)
        , x(_x)
        , y(_y)
        , width(_width)
        , height(_height)
        , taskbar(nullptr)
    {}
    
    bool b_primary;
    int x;
    int y;
    uint width;
    uint height;
    std::shared_ptr<class EshyWMTaskbar> taskbar;
};

class EshyWMWindow;

namespace WindowManager
{
    void initialize();
    void handle_events();
    void handle_preexisting_windows();

    extern Display* display;
    extern std::vector<std::shared_ptr<EshyWMWindow>> window_list;
    extern std::vector<std::shared_ptr<s_monitor_info>> monitors;
};