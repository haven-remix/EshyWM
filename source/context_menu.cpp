
#include "context_menu.h"
#include "eshywm.h"
#include "window_manager.h"
#include "button.h"

void EshyWMContextMenu::initialize_context_menu()
{
    context_menu_window = XCreateSimpleWindow(
        DISPLAY,
        ROOT,
        0,
        0,
        CONFIG->context_menu_width,
        150,
        0,
        0,
        CONFIG->context_menu_color
    );
    XSelectInput(DISPLAY, context_menu_window, SubstructureRedirectMask | SubstructureNotifyMask | VisibilityChangeMask);
    XGrabButton(DISPLAY, Button1, 0, context_menu_window, false, ButtonPressMask | ButtonReleaseMask, GrabModeSync, GrabModeAsync, None, None);
    XMapWindow(DISPLAY, context_menu_window);

    graphics_context_internal = XCreateGC(DISPLAY, context_menu_window, 0, 0);
}

void EshyWMContextMenu::show_context_menu(int x, int y)
{
    XMoveWindow(DISPLAY, context_menu_window, x, y);
    XMapWindow(DISPLAY, context_menu_window);
    raise_context_menu();
}

void EshyWMContextMenu::remove_context_menu()
{
    XUnmapWindow(DISPLAY, context_menu_window);
}

void EshyWMContextMenu::raise_context_menu()
{
    XRaiseWindow(DISPLAY, context_menu_window);
}

void EshyWMContextMenu::draw_context_menu()
{
    int i = 0;
    for(std::shared_ptr<Button> button : context_menu_buttons)
    {
        button->draw(10, (CONFIG->switcher_button_height * i) + 10);
        i++;
    }
}
