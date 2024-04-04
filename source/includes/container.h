
#pragma once

#include "util.h"

#include <X11/Xlib.h>

#include <string>

enum EDockLocation
{
    DL_NONE,
    DL_Top,
    DL_Bottom,
    DL_Left,
    DL_Right
};

struct Dock
{
    Window window;
    Output* parent_output = nullptr;
    Rect geometry = {0};
    EDockLocation dock_location = DL_NONE;
};

struct Workspace
{
    int num = -1;
    Output* parent_output = nullptr;

    bool b_is_active = false;
    //Space left after docks have been accounted for
    Rect geometry;

    //I am not convinced this is necessary, I can just find_if over window_list if I
    //really need to get all windows in a workspace. Since it wont be a frequent
    //operation the memory saving is likely worth it. 
    //std::vector<EshyWMWindow*> windows;
};

struct Output
{
    std::string name;
    Rect geometry = {0};
    Dock* top_dock = nullptr;
    Dock* bottom_dock = nullptr;
    Workspace* active_workspace = nullptr;

    void activate_workspace(Workspace* new_workspace);
    void deactivate_workspace();

    void add_dock(Dock* new_dock, EDockLocation location);
    void remove_dock(Dock* dock);
};