
#include "taskbar.h"
#include "eshywm.h"
#include "window_manager.h"
#include "config.h"
#include "window.h"

void EshyWMTaskbar::initialize_taskbar()
{
    const unsigned int height = DisplayHeight(DISPLAY, DefaultScreen(DISPLAY));
    taskbar_window = XCreateSimpleWindow(
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
    XSelectInput(DISPLAY, taskbar_window, SubstructureRedirectMask | SubstructureNotifyMask | VisibilityChangeMask);
    XGrabButton(DISPLAY, Button1, 0, taskbar_window, false, ButtonPressMask | ButtonReleaseMask, GrabModeSync, GrabModeAsync, None, None);
    XMapWindow(DISPLAY, taskbar_window);

    graphics_context_internal = XCreateGC(DISPLAY, taskbar_window, 0, 0);
}

void EshyWMTaskbar::update_taskbar_size(uint width, uint height)
{
    XMoveWindow(EshyWM::get_window_manager()->get_display(), taskbar_window, 0, height - EshyWM::get_current_config()->taskbar_height);
    XResizeWindow(EshyWM::get_window_manager()->get_display(), taskbar_window, width, EshyWM::get_current_config()->taskbar_height);
}

void EshyWMTaskbar::raise_taskbar()
{
    XRaiseWindow(DISPLAY, taskbar_window);
}

void EshyWMTaskbar::draw()
{
    int i = 0;
    for(auto const& [button, window] : taskbar_buttons)
    {
        button->set_position((CONFIG->taskbar_height * i) + 4, 2);
        button->draw();
        i++;
    }
}


void EshyWMTaskbar::add_button(std::shared_ptr<EshyWMWindow> associated_window)
{
    const Atom NET_WM_ICON = XInternAtom(DISPLAY, "_NET_WM_ICON", false);
    const Atom CARDINAL = XInternAtom(DISPLAY, "CARDINAL", false);

    Atom type_return;
    int format_return;
    unsigned long nitems_return;
    unsigned long bytes_after_return;
    unsigned char* data_return;

    XGetWindowProperty(DISPLAY, associated_window->get_window(), NET_WM_ICON, 0, 1, false, CARDINAL, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
    const int width = *(int*)data_return;
    XFree(data_return);

    XGetWindowProperty(DISPLAY, associated_window->get_window(), NET_WM_ICON, 1, 1, false, CARDINAL, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
    const int height = *(int*)data_return;
    XFree(data_return);

    XGetWindowProperty(DISPLAY, associated_window->get_window(), NET_WM_ICON, 2, width * height, false, CARDINAL, &type_return, &format_return, &nitems_return, &bytes_after_return, &data_return);
    
    uint32_t* img_data = new uint32_t[width * height];
    const ulong* ul = (ulong*)data_return;

    for(int i = 0; i < nitems_return; i++)
    {
        img_data[i] = (uint32_t)ul[i];
    }

    XFree(data_return);

    const rect size = {0, 0, EshyWM::get_current_config()->taskbar_height - 4, EshyWM::get_current_config()->taskbar_height - 4};
    std::shared_ptr<ImageButton> button = std::make_shared<ImageButton>(taskbar_window, size, imlib_create_image_using_data(width, height, img_data));
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