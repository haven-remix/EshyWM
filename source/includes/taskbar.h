
#pragma once

#include "util.h"
#include "menu_base.h"

#include <Imlib2.h>
#include <memory>
#include <vector>

#include <cairo/cairo.h>

class EshyWMTaskbar : public EshyWMMenuBase
{
public:

    EshyWMTaskbar(Rect _menu_geometry, Color _menu_color);
    ~EshyWMTaskbar();

    void update_taskbar_size(uint width, uint height);
    void update_button_positions();
    void show_taskbar(bool b_show);

    void add_button(class EshyWMWindow* associated_window, const Imlib_Image& icon);
    void remove_button(class EshyWMWindow* associated_window);

    void display_system_info();

    void on_taskbar_button_clicked(EshyWMWindow* window);

    const std::vector<std::shared_ptr<class WindowButton>>& get_taskbar_buttons() const {return taskbar_buttons;}
    std::vector<std::shared_ptr<class WindowButton>>& get_taskbar_buttons() {return taskbar_buttons;}

private:

    std::vector<std::shared_ptr<class WindowButton>> taskbar_buttons;

    cairo_surface_t* cairo_taskbar_surface;
    cairo_t* cairo_context;
};