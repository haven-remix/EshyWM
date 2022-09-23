#pragma once

extern "C" {
#include <X11/Xlib.h>
}

#include "util.h"

struct window_data
{
public:

    Window get_window() {return window;}
    class EshyWMWindow* get_eshywm_window() {return window_internal;}

    void update_internal_window_data(window_data* new_window_data);

    int get_horizontal_slot() {return horizontal_slot;}
    void set_horizontal_slot(int new_horizontal_slot) {horizontal_slot = new_horizontal_slot;}

    int get_vertical_slot() {return vertical_slot;}
    void set_vertical_slot(int new_vertical_slot) {vertical_slot = new_vertical_slot;}

    bool is_floating() {return b_floating;}
    void set_floating(bool new_b_floating) {b_floating = new_b_floating;}

    bool is_fullscreen() {return b_fullscreen;}
    void set_fullscreen(bool new_b_fullscreen) {b_fullscreen = new_b_fullscreen;}

private:

    Window window;
    class EshyWMWindow* window_internal;

    /**Starts a 0; in other words, 0 = slot one. This makes dealing with arrays easier. -1 denotes floting*/
    int horizontal_slot;
    int vertical_slot;

    bool b_floating;
    bool b_fullscreen;

public:

    window_data()
        : window(NULL)
        , window_internal(nullptr)
        , horizontal_slot(1)
        , vertical_slot(1)
        , b_floating(false)
        , b_fullscreen(false)
    {}

    window_data(Window _window, class EshyWMWindow* _window_internal)
        : window(_window)
        , window_internal(_window_internal)
        , horizontal_slot(1)
        , vertical_slot(1)
        , b_floating(false)
        , b_fullscreen(false)
    {}

    window_data(int _horizontal_slot, int _vertical_slot)
        : horizontal_slot(_horizontal_slot)
        , vertical_slot(_vertical_slot)
        , b_floating(false)
        , b_fullscreen(false)
    {}

    window_data(bool _b_floating)
        : horizontal_slot(1)
        , vertical_slot(1)
        , b_floating(_b_floating)
        , b_fullscreen(false)
    {}

    window_data(int _horizontal_slot, int _vertical_slot, bool _b_floating)
        : horizontal_slot(_horizontal_slot)
        , vertical_slot(_vertical_slot)
        , b_floating(_b_floating)
        , b_fullscreen(false)
    {}

    window_data(int _horizontal_slot, int _vertical_slot, bool _b_floating, bool _b_fullscreen)
        : horizontal_slot(_horizontal_slot)
        , vertical_slot(_vertical_slot)
        , b_floating(_b_floating)
        , b_fullscreen(_b_fullscreen)
    {}
};