#pragma once

extern "C" {
#include <X11/Xlib.h>
}

#include <vector>
#include <memory>

class EshyWMContextMenu
{
public:

    void initialize_context_menu();
    void show_context_menu(int x, int y);
    void remove_context_menu();
    void raise_context_menu();
    void draw_context_menu();

    Window get_context_menu_window() const {return context_menu_window;}

private:

    Window context_menu_window;
    GC graphics_context_internal;

    std::vector<std::shared_ptr<class Button>> context_menu_buttons;
};