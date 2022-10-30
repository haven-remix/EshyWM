
#include "button.h"
#include "eshywm.h"
#include "window_manager.h"
#include "config.h"

void Button::draw()
{
    XSetForeground(EshyWM::get_window_manager()->get_display(), drawable_graphics_context, button_color_normal);
    XFillRectangle(
        EshyWM::get_window_manager()->get_display(),
        drawable,
        drawable_graphics_context,
        button_size_and_location.x,
        button_size_and_location.y,
        button_size_and_location.width,
        button_size_and_location.height
    );
}

void Button::draw(int x, int y)
{
    set_position(x, y);
    XSetForeground(EshyWM::get_window_manager()->get_display(), drawable_graphics_context, button_color_normal);
    XFillRectangle(
        EshyWM::get_window_manager()->get_display(),
        drawable,
        drawable_graphics_context,
        x,
        y,
        button_size_and_location.width,
        button_size_and_location.height
    );
}

void Button::set_position(int x, int y)
{
    button_size_and_location.x = x;
    button_size_and_location.y = y;
}

void Button::set_size(unsigned int width, unsigned int height)
{
    button_size_and_location.width = width;
    button_size_and_location.height = height;
}

bool Button::is_hovered(int cursor_x, int cursor_y)
{
    if(cursor_x > button_size_and_location.x
    && cursor_x < button_size_and_location.x + button_size_and_location.width
    && cursor_y > button_size_and_location.y
    && cursor_y < button_size_and_location.y + button_size_and_location.height)
    {
        return true;
    }

    return false;
}