
#include "button.h"
#include "eshywm.h"
#include "window_manager.h"
#include "config.h"
#include "X11.h"

#include <Imlib2.h>
#include <X11/Xutil.h>

void Button::set_position(int x, int y)
{
    button_geometry.x = x;
    button_geometry.y = y;
}

void Button::set_size(uint width, uint height)
{
    button_geometry.width = width;
    button_geometry.height = height;
}

const bool Button::check_hovered(int cursor_x, int cursor_y) const
{
    if(cursor_x > button_geometry.x
    && cursor_x < button_geometry.x + button_geometry.width
    && cursor_y > button_geometry.y
    && cursor_y < button_geometry.y + button_geometry.height)
    {
        return true;
    }

    return false;
}

void Button::click()
{
    if(click_callback)
        click_callback(nullptr);
}


WindowButton::WindowButton(Window parent_window, const Rect& _button_geometry, const button_color_data& _background_color)
{
    button_geometry = _button_geometry;
    button_color = _background_color;

    button_window = X11::create_window(button_geometry, StructureNotifyMask | VisibilityChangeMask | EnterWindowMask | LeaveWindowMask | KeyPressMask | KeyReleaseMask, 0);
    X11::reparent_window(button_window, parent_window, {});
    X11::map_window(button_window);
    XSync(X11::get_display(), false);
}

WindowButton::~WindowButton()
{
    X11::unmap_window(button_window);
    X11::destroy_window(button_window);
}

void WindowButton::set_border_color(Color border_color)
{
    X11::set_border_color(button_window, border_color);
    XClearWindow(X11::get_display(), button_window);
    draw();
}

void WindowButton::set_position(int x, int y)
{
    button_geometry.x = x;
    button_geometry.y = y;
    X11::move_window(button_window, Pos{x, y});
}

void WindowButton::set_size(uint width, uint height)
{
    button_geometry.width = width;
    button_geometry.height = height;
    X11::resize_window(button_window, Size{width, height});
}

void WindowButton::on_update_state()
{
    const Color background_color = button_state == EButtonState::S_Hovered ? button_color.hovered 
        : button_state == EButtonState::S_Pressed ? button_color.pressed 
        : button_color.normal;
    
    X11::set_background_color(button_window, background_color);
    XClearWindow(X11::get_display(), button_window);
    draw();
}


ImageButton::~ImageButton()
{
    delete button_image;
}

void ImageButton::draw()
{
    button_image->draw(button_window, 0, 0, button_geometry.width, button_geometry.height);
}

void ImageButton::set_image(const Imlib_Image& new_image)
{
    delete button_image;
    button_image = new Image(new_image);
    draw();
}

