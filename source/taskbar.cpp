
#include "taskbar.h"
#include "eshywm.h"
#include "window_manager.h"
#include "config.h"
#include "window.h"

void EshyWMTaskbar::initialize_taskbar()
{
    const unsigned int height = DisplayHeight(DISPLAY, DefaultScreen(DISPLAY));
    taskbar = XCreateSimpleWindow(
        DISPLAY,
        ROOT,
        0,
        height - CONFIG->taskbar_height,
        WINDOW_MANAGER->get_display_width(),
        CONFIG->taskbar_height,
        0,
        0,
        CONFIG->taskbar_color
    );
    XSelectInput(DISPLAY, taskbar, SubstructureRedirectMask | SubstructureNotifyMask | VisibilityChangeMask);
    XGrabButton(DISPLAY, Button1, 0, taskbar, false, ButtonPressMask | ButtonReleaseMask, GrabModeSync, GrabModeAsync, None, None);
    XMapWindow(DISPLAY, taskbar);

    graphics_context_internal = XCreateGC(DISPLAY, taskbar, 0, 0);
}

void EshyWMTaskbar::update_taskbar_size(uint width, uint height)
{
    XMoveWindow(EshyWM::get_window_manager()->get_display(), taskbar, 0, height - EshyWM::get_current_config()->taskbar_height);
    XResizeWindow(EshyWM::get_window_manager()->get_display(), taskbar, width, EshyWM::get_current_config()->taskbar_height);
}

void EshyWMTaskbar::raise_taskbar()
{
    XRaiseWindow(DISPLAY, taskbar);
}

void EshyWMTaskbar::draw_taskbar()
{
    int i = 0;
    for(auto const& [button, window] : taskbar_buttons)
    {
        button->draw((EshyWM::get_current_config()->taskbar_height * i) + 4, 2);
        i++;
    }
}


void EshyWMTaskbar::add_button(std::shared_ptr<EshyWMWindow> associated_window)
{
    const rect size = {0, 0, EshyWM::get_current_config()->taskbar_height - 4, EshyWM::get_current_config()->taskbar_height - 4};
    std::shared_ptr<Button> button = std::make_shared<Button>(taskbar, graphics_context_internal, size);
    taskbar_buttons.emplace(button, associated_window);
}


void EshyWMTaskbar::check_taskbar_button_clicked(int cursor_x, int cursor_y)
{
    for(auto const& [button, window] : taskbar_buttons)
    {
        if(button->is_hovered(cursor_x, cursor_y))
        {
            window->minimize_window();
            XRaiseWindow(EshyWM::get_window_manager()->get_display(), window->get_frame());
            break;
        }
    }
}