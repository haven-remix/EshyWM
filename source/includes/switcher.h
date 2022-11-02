
#pragma once

extern "C" {
#include <X11/Xlib.h>
}

#include <memory>
#include <unordered_map>

class EshyWMSwitcher
{
public:

    void initialize_switcher();
    void show_switcher();
    void remove_switcher();
    void raise_switcher();
    void draw_switcher();

    void add_window_option(std::shared_ptr<class EshyWMWindow> associated_window);
    void confirm_choice();

    void check_close();

    Window get_switcher_window() const {return switcher_window;}
    
private:

    Window switcher_window;
    GC graphics_context_internal;

    std::unordered_map<std::shared_ptr<class Button>, std::shared_ptr<class EshyWMWindow>> switcher_window_options;
};