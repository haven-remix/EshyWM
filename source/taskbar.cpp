
#include "taskbar.h"
#include "eshywm.h"
#include "window_manager.h"
#include "config.h"
#include "window.h"
#include "button.h"
#include "system.h"

#include <algorithm>
#include <string>
#include <cstring>
#include <format>

#include <X11/Xatom.h>
#include <cairo/cairo-xlib.h>

void on_taskbar_button_clicked(std::shared_ptr<EshyWMWindow> window, void* null)
{
    Window returned_root;
    Window returned_parent;
    Window* top_level_windows;
    unsigned int num_top_level_windows;
    XQueryTree(DISPLAY, ROOT, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows);

    if(window->get_frame() != top_level_windows[num_top_level_windows - 1])
    {
        if(window->get_window_state() == WS_MINIMIZED)
            EshyWMWindow::toggle_minimize(window);

        XRaiseWindow(DISPLAY, window->get_frame());
        window->update_titlebar();
    }
    else EshyWMWindow::toggle_minimize(window);

    XFree(top_level_windows);
}


EshyWMTaskbar::EshyWMTaskbar(Rect _menu_geometry, Color _menu_color) : EshyWMMenuBase(_menu_geometry, _menu_color)
{
    const char* class_name = "eshywm_taskbar\0taskbar";
    Atom ATOM_CLASS = XInternAtom(DISPLAY, "WM_CLASS", False);
    XChangeProperty(DISPLAY, menu_window, ATOM_CLASS, XA_STRING, 8, PropModeReplace, reinterpret_cast<const unsigned char*>(class_name), strlen(class_name));
    XFlush(DISPLAY);

    XGrabButton(DISPLAY, Button1, AnyModifier, menu_window, false, ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, menu_window, None);

    cairo_taskbar_surface = cairo_xlib_surface_create(DISPLAY, menu_window, DefaultVisual(DISPLAY, 0), menu_geometry.width, menu_geometry.height);
    cairo_context = cairo_create(cairo_taskbar_surface);
}

EshyWMTaskbar::~EshyWMTaskbar()
{
    cairo_destroy(cairo_context);
    cairo_surface_destroy(cairo_taskbar_surface);
}

void EshyWMTaskbar::update_taskbar_size(uint width, uint height)
{
    set_position(0, height - EshyWMConfig::taskbar_height);
    set_size(width, EshyWMConfig::taskbar_height);

    display_system_info();
}

void EshyWMTaskbar::update_button_positions()
{
    for(int i = 0; i < taskbar_buttons.size(); i++)
    {
        taskbar_buttons[i]->set_position((EshyWMConfig::taskbar_height * i) + 4, 2);
        taskbar_buttons[i]->draw();
    }
}

void EshyWMTaskbar::show_taskbar(bool b_show)
{
    if(b_show)
        XMapWindow(DISPLAY, menu_window);
    else
        XUnmapWindow(DISPLAY, menu_window);
}


void EshyWMTaskbar::add_button(std::shared_ptr<EshyWMWindow> associated_window, const Imlib_Image& icon)
{
    const std::shared_ptr<ImageButton> button = std::make_shared<ImageButton>(
        menu_window, 
        Rect{0, 0, EshyWMConfig::taskbar_height - 4, EshyWMConfig::taskbar_height - 4}, 
        button_color_data{EshyWMConfig::taskbar_color, EshyWMConfig::taskbar_button_hovered_color, EshyWMConfig::taskbar_color}, 
        icon
    );
    button->get_data() = {associated_window, &on_taskbar_button_clicked};
    taskbar_buttons.push_back(button);
    update_button_positions();

    display_system_info();
}

void EshyWMTaskbar::remove_button(std::shared_ptr<EshyWMWindow> associated_window)
{
    for(int i = 0; i < taskbar_buttons.size(); i++)
    {
        if(taskbar_buttons[i]->get_data().associated_window == associated_window)
        {
            taskbar_buttons.erase(taskbar_buttons.begin() + i);
            update_button_positions();
            break;
        }
    }
}


void EshyWMTaskbar::display_system_info()
{
    XClearWindow(DISPLAY, menu_window);

    cairo_select_font_face(cairo_context, "Lato", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cairo_context, 16);
    cairo_set_source_rgb(cairo_context, 1.0f, 1.0f, 1.0f);

    int x_offset = menu_geometry.width;
    
    if(System::battery_percentage < 95)
    {
        cairo_move_to(cairo_context, x_offset -= 50.0f, EshyWMConfig::taskbar_height * 0.6);
        const std::string battery_percentage = std::to_string(System::battery_percentage) + "%";
        cairo_show_text(cairo_context, battery_percentage.c_str());
    }

    cairo_move_to(cairo_context, x_offset -= 100.0f, EshyWMConfig::taskbar_height * 0.6);
    const auto time = std::chrono::zoned_time("America/Los_Angeles", System::current_time);
    cairo_show_text(cairo_context, std::format("{:%r}", time).c_str());
}