
#include "switcher.h"
#include "eshywm.h"
#include "button.h"

#include <X11/Xutil.h>

void EshyWMSwitcher::initialize_switcher()
{
    switcher_window = XCreateSimpleWindow(
        DISPLAY,
        ROOT,
        (WINDOW_MANAGER->get_display_width() / 2) - (CONFIG->switcher_width / 2),
        (WINDOW_MANAGER->get_display_height() / 2) - (CONFIG->switcher_height / 2),
        CONFIG->switcher_width,
        CONFIG->switcher_height,
        0,
        0,
        CONFIG->switcher_color
    );
    XSelectInput(DISPLAY, switcher_window, SubstructureRedirectMask | SubstructureNotifyMask | VisibilityChangeMask);
    XGrabButton(DISPLAY, Button1, 0, switcher_window, false, ButtonPressMask | ButtonReleaseMask, GrabModeSync, GrabModeAsync, None, None);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Up), AnyModifier, switcher_window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Down), AnyModifier, switcher_window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Alt_L), AnyModifier, switcher_window, false, GrabModeAsync, GrabModeAsync);
    //XGrabKey(DISPLAY, XKeysymToKeycode(DISPLAY, XK_Tab), Mod1Mask, ROOT, false, GrabModeAsync, GrabModeAsync);

    graphics_context_internal = XCreateGC(DISPLAY, switcher_window, 0, 0);
}

void EshyWMSwitcher::show_switcher()
{
    if(!b_switcher_visible)
    {
        b_switcher_visible = true;
        XMapWindow(DISPLAY, switcher_window);
        raise_switcher();
    }
}

void EshyWMSwitcher::remove_switcher()
{
    if(b_switcher_visible)
    {
        b_switcher_visible = false;
        XUnmapWindow(DISPLAY, switcher_window);
    }
}

void EshyWMSwitcher::raise_switcher()
{
    XRaiseWindow(DISPLAY, switcher_window);
    XSetInputFocus(DISPLAY, switcher_window, RevertToPointerRoot, CurrentTime);
}

void EshyWMSwitcher::draw()
{
    int i = 0;
    for(auto const& [button, window] : switcher_window_options)
    {
        button->set_position(10, (CONFIG->switcher_button_height * i) + 10);
        button->draw();
        i++;
    }
}

void EshyWMSwitcher::next_option()
{
    std::cout << "next option" << std::endl;
}

void EshyWMSwitcher::add_window_option(std::shared_ptr<class EshyWMWindow> associated_window)
{
    const rect size = {0, 0, CONFIG->switcher_width - 20, CONFIG->switcher_button_height};
    std::shared_ptr<Button> button = std::make_shared<Button>(switcher_window, graphics_context_internal, size);
    switcher_window_options.emplace(button, associated_window);
}

void EshyWMSwitcher::confirm_choice()
{
    remove_switcher();
}