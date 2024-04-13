
#include "switcher.h"
#include "eshywm.h"
#include "button.h"
#include "window.h"
#include "X11.h"

#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xdamage.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <algorithm>
#include <string.h>
#include <cmath>

static const int get_button_x_position(const int i)
{
    int x = EshyWMConfig::switcher_button_padding;
    for(int j = 0; j < i; ++j)
    {
        x += SWITCHER->get_switcher_window_options()[j].button->get_button_geometry().width;
        x += EshyWMConfig::switcher_button_padding;
    }
    return x;
}


EshyWMSwitcher::EshyWMSwitcher(Rect _menu_geometry, Color _menu_color) : EshyWMMenuBase(_menu_geometry, _menu_color), selected_option(0)
{
    const char* class_name = "eshywm_switcher\0switcher";
    X11::change_window_property(menu_window, X11::atoms.window_class, XA_STRING, 8, (const unsigned char*)class_name);
    XFlush(X11::get_display());

    X11::grab_button(Button1, AnyModifier, menu_window, ButtonReleaseMask);
    
    /**
     * IMPORTANT: Keys do not send a release event if the key was registered after being pressed.
     * 
     * I cannot grab alt with GrabModeSync because then there is no release event (I do not know why).
     * I cannot grab alt with GrabModeAsync because then it is not passed into any windows. I have tried XAllowEvents and XSendEvent.
     * I cannot grab alt after a Alt-Tab press becase then a release event will not be triggered.
     * I have to grab these asynchronously and enable KeyReleaseMask in ROOT. The issue is I now get and have to handle the event for EVERY key release, not just Alt.
    */
    X11::grab_key(XK_Tab, Mod1Mask | 0, X11::get_root_window());
}

void EshyWMSwitcher::show()
{
    X11::map_window(menu_window);
    raise(true);
    b_menu_active = true;
}

void EshyWMSwitcher::raise(bool b_set_focus)
{
    update_button_positions();
    EshyWMMenuBase::raise(b_set_focus);
}

void EshyWMSwitcher::button_clicked(int x_root, int y_root)
{
    for(int i = 0; i < switcher_window_options.size(); i++)
    {
        if(switcher_window_options[i].button->check_hovered(x_root, y_root))
        {
            selected_option = i;
            confirm_choice();
            break;
        }
    }
}

void EshyWMSwitcher::update_button_positions()
{
    uint width = EshyWMConfig::switcher_button_padding;
    for(window_button_pair pair : switcher_window_options)
    {
        width += pair.button->get_button_geometry().width;
        width += EshyWMConfig::switcher_button_padding;
    }

    const uint height = EshyWMConfig::switcher_button_height + (EshyWMConfig::switcher_button_padding * 2);
    
    set_size(width, height);

    const Pos cursor_position = X11::get_cursor_position();

    if(Output* output = output_at_position(cursor_position.x, cursor_position.y))
        set_position(center_x(output, width), center_y(output, height));
    else
        set_position(center_x(EshyWM::window_manager->outputs[0], width), center_y(EshyWM::window_manager->outputs[0], height));

    for(int i = 0; i < switcher_window_options.size(); i++)
    {
        switcher_window_options[i].button->set_position(get_button_x_position(i), EshyWMConfig::switcher_button_padding);
        switcher_window_options[i].button->draw();
    }
}

void EshyWMSwitcher::update_switcher_window_options()
{
    std::vector<window_button_pair> old_switcher_window_options = switcher_window_options;

    // switcher_window_options.clear();

    // for(std::shared_ptr<EshyWMWindow> window : WindowManager::window_list)
    // {
    //     for(window_button_pair pair : old_switcher_window_options)
    //     {
    //         if(pair.window == window)
    //             switcher_window_options.push_back(pair);
    //     }
    // }
}


void EshyWMSwitcher::add_window_option(EshyWMWindow* associated_window, const Imlib_Image& icon)
{
    imlib_context_set_image(icon);
    const int height = imlib_image_get_height();
    const int width = imlib_image_get_width();

    const float scale = 140.0f / height;

    const Rect size = {0, 0, (uint)std::round(width * scale), (uint)std::round(height * scale)};
    const button_color_data color = {EshyWMConfig::switcher_button_color, EshyWMConfig::switcher_button_color, EshyWMConfig::switcher_button_color};

    std::shared_ptr<ImageButton> button = std::make_shared<ImageButton>(menu_window, size, color, icon);
    button->set_border_color(EshyWMConfig::switcher_button_border_color);
    
    switcher_window_options.insert(switcher_window_options.begin(), {associated_window, button});
    update_button_positions();
}

void EshyWMSwitcher::remove_window_option(EshyWMWindow* associated_window)
{
    for(int i = 0; i < switcher_window_options.size(); i++)
    {
        if(switcher_window_options[i].window == associated_window)
        {
            switcher_window_options.erase(switcher_window_options.begin() + i);
            update_button_positions();
            break;
        }
    }
}

void EshyWMSwitcher::next_option()
{
    selected_option = std::max(selected_option + 1, 0);
    if(selected_option >= switcher_window_options.size())
    {
        selected_option = 0;
    }
    select_option(selected_option);
}

void EshyWMSwitcher::select_option(int i)
{
    //Deselect previous
    if(i == 0 && switcher_window_options[switcher_window_options.size() - 1].button)
    {
        X11::move_window(switcher_window_options[switcher_window_options.size() - 1].button->get_window(), Pos{get_button_x_position(switcher_window_options.size() - 1), (int)EshyWMConfig::switcher_button_padding});
        X11::set_border_width(switcher_window_options[switcher_window_options.size() - 1].button->get_window(), 0);
    }
    else if(i > 0 && switcher_window_options[i - 1].button)
    {
        X11::move_window(switcher_window_options[i - 1].button->get_window(), Pos{get_button_x_position(i - 1), (int)EshyWMConfig::switcher_button_padding});
        X11::set_border_width(switcher_window_options[i - 1].button->get_window(), 0);
    }

    if(switcher_window_options[i].button)
    {
        X11::move_window(switcher_window_options[i].button->get_window(), Pos{get_button_x_position(i), (int)EshyWMConfig::switcher_button_padding});
        X11::set_border_width(switcher_window_options[i].button->get_window(), 2);
    }
}

void EshyWMSwitcher::handle_option_chosen(const int i)
{
    const X11::WindowTree window_tree = X11::query_window_tree(X11::get_root_window());

    if(switcher_window_options[i].window->get_frame() != window_tree.windows[window_tree.windows.size() - 1])
    {
        if(switcher_window_options[i].window->get_window_state() == WS_MINIMIZED)
            switcher_window_options[i].window->minimize_window(false);

        EshyWM::window_manager->focus_window(switcher_window_options[i].window, true);
        switcher_window_options[i].window->update_titlebar();
    }

    auto it = switcher_window_options.begin() + i;
    std::rotate(switcher_window_options.begin(), it, it + 1);
}


void EshyWMSwitcher::confirm_choice()
{
    handle_option_chosen(selected_option);
    remove();
    selected_option = 0;
}