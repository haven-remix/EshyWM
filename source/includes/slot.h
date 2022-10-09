
#pragma once

#include "util.h"

extern "C" {
#include <X11/Xlib.h>
}

#include <vector>

class slot_child
{
public:

    virtual void set_preferred_size(size<int> new_preferred_size) {preferred_size = new_preferred_size;}
    size<int> get_preferred_size() const {return preferred_size;}

    virtual void set_preferred_position(Vector2D<int> new_preferred_position) {preferred_position = new_preferred_position;}
    Vector2D<int> get_preferred_position() const {return preferred_position;}

private:

    size<int> preferred_size;
    Vector2D<int> preferred_position;
};

/**
 * A chainable class to represent a slot
*/
class slot : public slot_child
{
public:

    slot() {}

    slot(bool _b_horizontal)
        : b_horizontal(_b_horizontal)
    {}

    void realign_content(slot_child* child_to_favor = nullptr);
    void first_realign_content();

    void add_slot(slot_child* new_slot);
    void remove_slot(slot_child* slot_to_remove);
    std::vector<slot_child*> get_content() const {return content;}

private:

    /**Either horizontal or vertical*/
    bool b_horizontal;

    std::vector<slot_child*> content;
};