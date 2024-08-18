
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
    std::shared_ptr<Output> parent_output = nullptr;
    Rect geometry = {0};
    EDockLocation dock_location = DL_NONE;
};

struct Workspace
{
    int num = -1;
    std::shared_ptr<Output> parent_output = nullptr;

    bool b_is_active = false;
    //Space left after docks have been accounted for
    Rect geometry;
};

struct Output
{
    std::string name;
    Rect geometry = {0};
    std::shared_ptr<Dock> top_dock = nullptr;
    std::shared_ptr<Dock> bottom_dock = nullptr;
    std::shared_ptr<Workspace> active_workspace = nullptr;

    void activate_workspace(std::shared_ptr<Workspace> new_workspace);
    void deactivate_workspace();

    void add_dock(std::shared_ptr<Dock> new_dock, EDockLocation location);
    void remove_dock(std::shared_ptr<Dock> dock);
};