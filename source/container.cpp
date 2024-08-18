
#include "container.h"
#include "window_manager.h"
#include "window.h"
#include "eshywm.h"

#include <ranges>
#include <algorithm>
#include <assert.h>

void Output::activate_workspace(std::shared_ptr<Workspace> new_workspace)
{
    assert(new_workspace);
    if(new_workspace->b_is_active)
        return;

    deactivate_workspace();
    
    active_workspace = new_workspace;
    active_workspace->b_is_active = true;

    //Calculate geometry for this workspace
    active_workspace->geometry = geometry;
    
    if(top_dock)
    {
        active_workspace->geometry.y += top_dock->geometry.height;
        active_workspace->geometry.height -= top_dock->geometry.height;
    }

    if(bottom_dock)
        active_workspace->geometry.height -= bottom_dock->geometry.height;

    //Raise all windows of this workspace
    auto in_active_workspace = [new_workspace = new_workspace](auto window){return window->parent_workspace == new_workspace;};
    for(auto window : EshyWM::window_manager->window_list | std::views::filter(in_active_workspace))
    {
        window->minimize_window(false);
    }
}

void Output::deactivate_workspace()
{
    if(!active_workspace)
        return;

    auto in_active_workspace = [active_workspace = active_workspace](auto window){return window->parent_workspace == active_workspace;};
    for(auto window : EshyWM::window_manager->window_list | std::views::filter(in_active_workspace))
    {
        window->minimize_window(true);
    }

    active_workspace->b_is_active = false;
    active_workspace = nullptr;
}

void Output::add_dock(std::shared_ptr<Dock> new_dock, EDockLocation location)
{
    switch (location)
    {
    case DL_Top:
        top_dock = new_dock;
        break;
    case DL_Bottom:
        bottom_dock = new_dock;
        break;
    };

    //Recalculate geometry
    active_workspace->geometry = geometry;

    if(top_dock)
    {
        active_workspace->geometry.y += top_dock->geometry.height;
        active_workspace->geometry.height -= top_dock->geometry.height;
    }

    if(bottom_dock)
        active_workspace->geometry.height -= bottom_dock->geometry.height;
    
    //Propogate geometry change to all windows in active_workspace
    auto in_active_workspace = [active_workspace = active_workspace](auto window){return window->parent_workspace == active_workspace;};
    for(auto window : EshyWM::window_manager->window_list | std::views::filter(in_active_workspace))
    {
        if(window->get_window_state() == WS_MAXIMIZED)
        {
            window->maximize_window(false);
            window->maximize_window(true);
        }
    }
}

void Output::remove_dock(std::shared_ptr<Dock> dock)
{
    switch (dock->dock_location)
    {
    case DL_Top:
        top_dock = nullptr;
        break;
    case DL_Bottom:
        bottom_dock = nullptr;
        break;
    };

    //Recalculate geometry
    active_workspace->geometry = geometry;

    if(top_dock)
    {
        active_workspace->geometry.y += top_dock->geometry.height;
        active_workspace->geometry.height -= top_dock->geometry.height;
    }

    if(bottom_dock)
        active_workspace->geometry.height -= bottom_dock->geometry.height;

    //Propogate geometry change to all windows in active_workspace
    auto in_active_workspace = [active_workspace = active_workspace](auto window){return window->parent_workspace == active_workspace;};
    for(auto window : EshyWM::window_manager->window_list | std::views::filter(in_active_workspace))
    {
        if(window->get_window_state() == WS_MAXIMIZED)
        {
            window->maximize_window(false);
            window->maximize_window(true);
        }
    }
}
