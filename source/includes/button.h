
#pragma once

extern "C" {
#include <X11/Xlib.h>
}

#include "util.h"

class Button
{
public:

    Button(const Drawable _drawable, const GC _drawable_graphics_context)
        : drawable(_drawable)
        , drawable_graphics_context(_drawable_graphics_context)
        , button_color_normal(0x000000)
        , button_color_hovered(0x000000)
        , button_color_pressed(0x000000)
    {}

    Button(const Drawable _drawable, const GC _drawable_graphics_context, rect _button_size_and_location)
        : drawable(_drawable)
        , drawable_graphics_context(_drawable_graphics_context)
        , button_size_and_location(_button_size_and_location)
        , button_color_normal(0x000000)
        , button_color_hovered(0x000000)
        , button_color_pressed(0x000000)
    {}

    Button(
        const Drawable _drawable,
        const GC _drawable_graphics_context,
        rect _button_size_and_location,
        unsigned long _button_color_normal,
        unsigned long _button_color_hovered,
        unsigned long _button_color_pressed
    )
        : drawable(_drawable)
        , drawable_graphics_context(_drawable_graphics_context)
        , button_size_and_location(_button_size_and_location)
        , button_color_normal(_button_color_normal)
        , button_color_hovered(_button_color_hovered)
        , button_color_pressed(_button_color_pressed)
    {}

    void draw();
    void draw(int x, int y);
    bool is_hovered(int cursor_x, int cursor_y);

    void set_position(int x, int y);
    void set_size(unsigned int width, unsigned int height);

private:

    const Drawable drawable;
    const GC drawable_graphics_context;

    rect button_size_and_location;
    unsigned long button_color_normal;
    unsigned long button_color_hovered;
    unsigned long button_color_pressed;
};