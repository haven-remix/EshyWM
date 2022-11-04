
#pragma once

extern "C" {
#include <X11/Xlib.h>
}

#include "button.h"

#include <memory>
#include <unordered_map>

class EshyWMTaskbar
{
public:

    void initialize_taskbar();
    void update_taskbar_size(uint width, uint height);
    void raise_taskbar();
    void draw_taskbar();

    void add_button(std::shared_ptr<class EshyWMWindow> associated_window);

    void check_taskbar_button_clicked(int cursor_x, int cursor_y);

    Window get_taskbar_window() const {return taskbar_window;}

private:

    Window taskbar_window;
    GC graphics_context_internal;

    std::unordered_map<std::shared_ptr<Button>, std::shared_ptr<class EshyWMWindow>> taskbar_buttons;
};