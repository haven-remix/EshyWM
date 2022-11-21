
#pragma once

#include "util.h"
#include "menu_base.h"
#include "button.h"

#include <memory>
#include <vector>

struct window_button_pair
{
    std::shared_ptr<class EshyWMWindow> window;
    std::shared_ptr<ImageButton> button;
};

class EshyWMTaskbar : public EshyWMMenuBase
{
public:

    EshyWMTaskbar(rect _menu_geometry, Color _menu_color);
    void update_taskbar_size(uint width, uint height);
    void update_button_positions();

    void add_button(std::shared_ptr<class EshyWMWindow> associated_window);
    void remove_button(std::shared_ptr<class EshyWMWindow> associated_window);

    void check_taskbar_button_clicked(int cursor_x, int cursor_y);

    const std::vector<window_button_pair> get_taskbar_buttons() const {return taskbar_buttons;}

private:

    std::vector<window_button_pair> taskbar_buttons;
};