
#include "menu_base.h"
#include "eshywm.h"
#include "window_manager.h"
#include "X11.h"

#include <X11/Xutil.h>

EshyWMMenuBase::EshyWMMenuBase(Rect _menu_geometry, Color _menu_color) : menu_geometry(_menu_geometry), menu_color(_menu_color)
{
    menu_window = XCreateSimpleWindow(X11::get_display(), X11::get_root_window(), menu_geometry.x, menu_geometry.y, menu_geometry.width, menu_geometry.height, 0, 0, menu_color);
    XSelectInput(X11::get_display(), menu_window, KeyReleaseMask | SubstructureRedirectMask | SubstructureNotifyMask | VisibilityChangeMask);
    
    graphics_context_internal = XCreateGC(X11::get_display(), menu_window, 0, 0);
}

void EshyWMMenuBase::show()
{
    X11::map_window(menu_window);
    raise();
    b_menu_active = true;
}

void EshyWMMenuBase::remove()
{
    X11::unmap_window(menu_window);
    b_menu_active = false;
}

void EshyWMMenuBase::raise(bool b_set_focus)
{
    X11::raise_window(menu_window);

    if(b_set_focus)
    {
        XSetInputFocus(X11::get_display(), menu_window, RevertToPointerRoot, CurrentTime);
    }
}

void EshyWMMenuBase::draw()
{

}

void EshyWMMenuBase::set_position(int x, int y)
{
    menu_geometry.x = x;
    menu_geometry.y = y;
    X11::move_window(menu_window, Pos{x, y});
}

void EshyWMMenuBase::set_size(uint width, uint height)
{
    menu_geometry.width = width;
    menu_geometry.height = height;
    X11::resize_window(menu_window, Size{width, height});
}
