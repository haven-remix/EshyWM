
#include "switcher.h"
#include "eshywm.h"
#include "button.h"
#include "window.h"

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


EshyWMSwitcher::EshyWMSwitcher(rect _menu_geometry, Color _menu_color) : EshyWMMenuBase(_menu_geometry, _menu_color), selected_option(0)
{
    const char* class_name = "eshywm_switcher\0switcher";
    Atom ATOM_CLASS = XInternAtom(DISPLAY, "WM_CLASS", False);
    XChangeProperty(DISPLAY, menu_window, ATOM_CLASS, XA_STRING, 8, PropModeReplace, reinterpret_cast<const unsigned char*>(class_name), strlen(class_name));
    XFlush(DISPLAY);

    XGrabButton(DISPLAY, Button1, AnyModifier, menu_window, false, ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Tab), Mod4Mask, ROOT, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Alt_L), AnyModifier, ROOT, false, GrabModeAsync, GrabModeAsync);
}

void EshyWMSwitcher::show()
{
    XMapWindow(DISPLAY, menu_window);
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

    Window window_return;
    int root_x;
    int root_y;
    int others;
    uint mask_return;

    XQueryPointer(DISPLAY, ROOT, &window_return, &window_return, &root_x, &root_y, &others, &others, &mask_return);

    std::shared_ptr<s_monitor_info> monitor;
    if(position_in_monitor(root_x, root_y, &monitor))
        set_position(CENTER_W(monitor, width), CENTER_H(monitor, height));
    else
        set_position(CENTER_W(WindowManager::monitors[0], width), CENTER_H(WindowManager::monitors[0], height));
    
    for(int i = 0; i < switcher_window_options.size(); i++)
    {
        switcher_window_options[i].button->set_position(get_button_x_position(i), EshyWMConfig::switcher_button_padding);
        switcher_window_options[i].button->draw();
    }
}

void EshyWMSwitcher::update_switcher_window_options()
{
    std::vector<window_button_pair> old_switcher_window_options = switcher_window_options;

    switcher_window_options.clear();

    for(std::shared_ptr<EshyWMWindow> window : WindowManager::window_list)
    {
        for(window_button_pair pair : old_switcher_window_options)
        {
            if(pair.window == window)
                switcher_window_options.push_back(pair);
        }
    }   
}


void EshyWMSwitcher::add_window_option(std::shared_ptr<EshyWMWindow> associated_window, const Imlib_Image& icon)
{
    imlib_context_set_image(icon);
    const int height = imlib_image_get_height();
    const int width = imlib_image_get_width();

    const float scale = 140.0f / height;

    const rect size = {0, 0, (uint)std::round(width * scale), (uint)std::round(height * scale)};
    const button_color_data color = {EshyWMConfig::switcher_button_color, EshyWMConfig::switcher_button_color, EshyWMConfig::switcher_button_color};

    std::shared_ptr<ImageButton> button = std::make_shared<ImageButton>(menu_window, size, color, icon);
    button->set_border_color(EshyWMConfig::switcher_button_border_color);
    
    switcher_window_options.insert(switcher_window_options.begin(), {associated_window, button});
    update_button_positions();
}

void EshyWMSwitcher::remove_window_option(std::shared_ptr<EshyWMWindow> associated_window)
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
        XMoveWindow(DISPLAY, switcher_window_options[switcher_window_options.size() - 1].button->get_window(), get_button_x_position(switcher_window_options.size() - 1), EshyWMConfig::switcher_button_padding);
        XSetWindowBorderWidth(DISPLAY, switcher_window_options[switcher_window_options.size() - 1].button->get_window(), 0);
    }
    else if(i > 0 && switcher_window_options[i - 1].button)
    {
        XMoveWindow(DISPLAY, switcher_window_options[i - 1].button->get_window(), get_button_x_position(i - 1), EshyWMConfig::switcher_button_padding);
        XSetWindowBorderWidth(DISPLAY, switcher_window_options[i - 1].button->get_window(), 0);
    }

    if(switcher_window_options[i].button)
    {
        XMoveWindow(DISPLAY, switcher_window_options[i].button->get_window(), get_button_x_position(i), EshyWMConfig::switcher_button_padding);
        XSetWindowBorderWidth(DISPLAY, switcher_window_options[i].button->get_window(), 2);
    }
}

void EshyWMSwitcher::handle_option_chosen(const int i)
{
    Window returned_root;
    Window returned_parent;
    Window* top_level_windows;
    unsigned int num_top_level_windows;
    XQueryTree(DISPLAY, ROOT, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows);

    if(switcher_window_options[i].window->get_frame() != top_level_windows[num_top_level_windows - 1])
    {
        if(switcher_window_options[i].window->get_window_state() == WS_MINIMIZED)
            switcher_window_options[i].window->minimize_window(false);

        XRaiseWindow(DISPLAY, switcher_window_options[i].window->get_frame());
        XSetInputFocus(DISPLAY, switcher_window_options[i].window->get_window(), RevertToPointerRoot, CurrentTime);
        switcher_window_options[i].window->update_titlebar();
    }

    XFree(top_level_windows);

    auto it = switcher_window_options.begin() + i;
    std::rotate(switcher_window_options.begin(), it, it + 1);
}


void EshyWMSwitcher::confirm_choice()
{
    handle_option_chosen(selected_option);
    remove();
    selected_option = 0;
}