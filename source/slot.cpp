
extern "C" {
#include <X11/Xlib.h>
}

#include "slot.h"
#include "eshywm.h"
#include "window_manager.h"

void slot::realign_content(slot_child* child_to_favor)
{
    const int content_width = b_horizontal ? get_preferred_size().width / content.size() : get_preferred_size().width;
    const int content_height = b_horizontal ? get_preferred_size().height : get_preferred_size().height / content.size();
    const size<int> slot_size(content_width, content_height);
    Vector2D<int> slot_position(0, 0);

    for(slot_child* content_slot : content)
    {
        //content_slot->set_preferred_size(slot_size);
        if(EshyWMWindow* window = dynamic_cast<EshyWMWindow*>(content_slot))
        {
            window->resize_window_absolute(content_slot->get_preferred_size());
            XMoveWindow(EshyWM::get_window_manager()->get_display(), window->get_frame(), slot_position.x, slot_position.y);
        }
        else if (slot* s = dynamic_cast<slot*>(content_slot))
        {
            s->realign_content(child_to_favor);
        }
        slot_position += slot_size;
    }
}

void slot::first_realign_content()
{
    Display* const display = EshyWM::get_window_manager()->get_display();
    const size<int> display_size(DisplayWidth(display, DefaultScreen(display)), DisplayHeight(display, DefaultScreen(display)));
    const size<int> slot_size(
        b_horizontal ? display_size.width / content.size() : display_size.width,
        b_horizontal ? display_size.height : display_size.height / content.size()
    );
    Vector2D<int> slot_position(0, 0);

    for(slot_child* content_slot : content)
    {
        content_slot->set_preferred_size(slot_size);
        if(EshyWMWindow* window = dynamic_cast<EshyWMWindow*>(content_slot))
        {
            window->resize_window_absolute(content_slot->get_preferred_size());
            XMoveWindow(EshyWM::get_window_manager()->get_display(), window->get_frame(), b_horizontal ? slot_position.x : 0, b_horizontal ? 0 : slot_position.y);
        }
        else if (slot* s = dynamic_cast<slot*>(content_slot))
        {
            s->first_realign_content();
        }
        slot_position += slot_size;
    }
}


void slot::add_slot(slot_child* new_slot)
{
    content.push_back(new_slot);
}

void slot::remove_slot(slot_child* slot_to_remove)
{
    remove(content.begin(), content.end(), slot_to_remove);
}