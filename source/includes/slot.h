
#pragma once

#include "util.h"

extern "C" {
#include <X11/Xlib.h>
}

#include <vector>
#include <algorithm>

class slot_child
{
public:

    virtual void set_preferred_size(Vector2D<int> new_preferred_size)
    {
        preferred_size.x = std::max(new_preferred_size.x, 1);
        preferred_size.y = std::max(new_preferred_size.y, 1);
    }
    Vector2D<int> get_preferred_size() const {return preferred_size;}

    virtual void set_preferred_position(Vector2D<int> new_preferred_position) {preferred_position = new_preferred_position;}
    Vector2D<int> get_preferred_position() const {return preferred_position;}

protected:

    Vector2D<int> preferred_size;
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
    {
        set_preferred_size(Vector2D<int>(0, 0));
        set_preferred_position(Vector2D<int>(0, 0));
    }

    void realign_content(slot_child* child_to_favor = nullptr);

    void add_slot(slot_child* new_slot);
    void remove_slot(slot_child* slot_to_remove);
    void move_slot(slot_child* slot_to_move, int move_amount);
    std::vector<slot_child*> get_content() const {return content;}
    std::vector<slot_child*>& get_content_r() {return content;}
    bool is_horizontal() const {return b_horizontal;}

private:

    /**Either horizontal or vertical*/
    bool b_horizontal;

    std::vector<slot_child*> content;

    void realign_equal(slot_child* child_to_favor);
    void realign_adjustive(slot_child* child_to_favor);
    void realign_adjustive_reverse(slot_child* child_to_favor);
};