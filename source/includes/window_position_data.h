#pragma once

extern "C" {
#include <X11/Xlib.h>
}

#include "util.h"

/**
 * Stores all information about the widnow (including window itself). Also acts as a getter for important inforamtion.
*/
struct window_position_data
{
public:

    /**Getters*/
    int get_horizontal_slot() {return horizontal_slot;}
    int get_vertical_slot() {return vertical_slot;}
    bool is_floating() {return b_floating;}
    bool is_fullscreen() {return b_fullscreen;}

    /**Setters*/
    inline void set_horizontal_slot(int new_horizontal_slot) {horizontal_slot = new_horizontal_slot;}
    inline void set_vertical_slot(int new_vertical_slot) {vertical_slot = new_vertical_slot;}
    inline void set_floating(bool new_b_floating) {b_floating = new_b_floating;}
    inline void set_fullscreen(bool new_b_fullscreen) {b_fullscreen = new_b_fullscreen;}

private:

    /**Starts a 0; in other words, 0 = slot one. This makes dealing with arrays easier. -1 denotes floting*/
    int horizontal_slot;
    int vertical_slot;

    bool b_floating;
    bool b_fullscreen;

public:

    /**Constructors*/

    window_position_data()
        : horizontal_slot(1)
        , vertical_slot(1)
        , b_floating(false)
        , b_fullscreen(false)
    {}

    window_position_data(int _horizontal_slot, int _vertical_slot)
        : horizontal_slot(_horizontal_slot)
        , vertical_slot(_vertical_slot)
        , b_floating(false)
        , b_fullscreen(false)
    {}

    window_position_data(bool _b_floating)
        : horizontal_slot(1)
        , vertical_slot(1)
        , b_floating(_b_floating)
        , b_fullscreen(false)
    {}

    window_position_data(int _horizontal_slot, int _vertical_slot, bool _b_floating)
        : horizontal_slot(_horizontal_slot)
        , vertical_slot(_vertical_slot)
        , b_floating(_b_floating)
        , b_fullscreen(false)
    {}

    window_position_data(int _horizontal_slot, int _vertical_slot, bool _b_floating, bool _b_fullscreen)
        : horizontal_slot(_horizontal_slot)
        , vertical_slot(_vertical_slot)
        , b_floating(_b_floating)
        , b_fullscreen(_b_fullscreen)
    {}
};