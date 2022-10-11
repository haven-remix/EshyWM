
extern "C" {
#include <X11/Xlib.h>
}

#include "slot.h"
#include "eshywm.h"
#include "window_manager.h"
#include "util.h"

#include <algorithm>

void slot::realign_content(slot_child* child_to_favor)
{
    if(EshyWM::get_window_manager()->get_manager_data()->window_tile_mode == EWindowTileMode::WTM_Equal)
    {
        realign_equal(child_to_favor);
    }
    else if(EshyWM::get_window_manager()->get_manager_data()->window_tile_mode == EWindowTileMode::WTM_Adjustive)
    {
        realign_adjustive(child_to_favor);
    }
}

void slot::realign_equal(slot_child* child_to_favor)
{
    const Vector2D<int> slot_size(
        b_horizontal ? get_preferred_size().x / content.size() : get_preferred_size().x,
        b_horizontal ? get_preferred_size().y : get_preferred_size().y / content.size()
    );

    for(int i = 0; i < content.size(); i++)
    {
        content[i]->set_preferred_size(slot_size);
        if(EshyWMWindow* window = dynamic_cast<EshyWMWindow*>(content[i]))
        {
            window->resize_window_absolute(content[i]->get_preferred_size());
            XMoveWindow(
                EshyWM::get_window_manager()->get_display(),
                window->get_frame(),
                b_horizontal ? (slot_size.x * i) : 0,
                b_horizontal ? 0 : (slot_size.y * i)
            );
        }
        else if (slot* s = dynamic_cast<slot*>(content[i]))
        {
            s->realign_content(child_to_favor);
        }
    }
}

void slot::realign_adjustive(slot_child* child_to_favor)
{
    const Vector2D<int> normal_slot_size(
        b_horizontal ? (get_preferred_size().x - child_to_favor->get_preferred_size().x) / content.size() : 0,
        b_horizontal ? 0 : (get_preferred_size().y - child_to_favor->get_preferred_size().y) / content.size()
    );

    Vector2D<int> slot_position(0, 0);
    for(slot_child* content_slot : content)
    {
        const Vector2D<int> slot_size = content_slot->get_preferred_size();
        if(EshyWMWindow* window = dynamic_cast<EshyWMWindow*>(content_slot))
        {
            window->resize_window_absolute(slot_size);
            XMoveWindow(
                EshyWM::get_window_manager()->get_display(),
                window->get_frame(),
                b_horizontal ? slot_position.x : 0,
                b_horizontal ? 0 : slot_position.y
            );
        }
        else if (slot* s = dynamic_cast<slot*>(content_slot))
        {
            s->realign_content(child_to_favor);
        }
        slot_position += slot_size;
    }
}

void slot::realign_adjustive_reverse(slot_child* child_to_favor)
{
    Vector2D<int> slot_position(preferred_position.x + preferred_size.x, preferred_position.y + preferred_size.y);
    for(std::vector<slot_child*>::reverse_iterator it = content.rbegin(); it != content.rend(); it++)
    {
        Vector2D<int> slot_size = (*it)->get_preferred_size();
        if(it == content.rend())
        {
            slot_size = slot_position;
        }

        if(EshyWMWindow* window = dynamic_cast<EshyWMWindow*>(*it))
        {
            window->resize_window_absolute(slot_size);
            XMoveWindow(
                EshyWM::get_window_manager()->get_display(),
                window->get_frame(),
                b_horizontal ? slot_position.x - slot_size.x : 0,
                b_horizontal ? 0 : slot_position.y - slot_size.y
            );
        }
        else if (slot* s = dynamic_cast<slot*>(*it))
        {
            s->realign_content(child_to_favor);
        }
        slot_position -= slot_size;
    }
}


void slot::add_slot(slot_child* new_slot)
{
    content.push_back(new_slot);
}

void slot::remove_slot(slot_child* slot_to_remove)
{
    for(int i = 0; i < content.size(); i++)
    {
        if(content[i] = slot_to_remove)
        {
            content.erase(content.begin() + i);
            break;
        }
    }
}

void slot::move_slot(slot_child* slot_to_move, int move_amount)
{
    std::vector<slot_child*>::iterator i = std::find(content.begin(), content.end(), slot_to_move);

    if(i != content.cend())
    {
        const int old_index = std::distance(content.begin(), i);
        i = (i + move_amount == content.cend()) ? content.begin() : (i == content.begin() && move_amount < 0) ? content.end() - 1 : i + move_amount;
        const int new_index = std::distance(content.begin(), i);

        if (old_index > new_index)
        {
            std::rotate(content.rend() - old_index - 1, content.rend() - old_index, content.rend() - new_index);
        }
        else
        {
            std::rotate(content.begin() + old_index, content.begin() + old_index + 1, content.begin() + new_index + 1);
        } 
    }
}