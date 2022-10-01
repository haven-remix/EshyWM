
#include "window_manager.h"
#include "window.h"
#include "config.h"
#include "eshywm.h"

extern "C" {
#include <X11/Xutil.h>
}

#include <glog/logging.h>


EshyWMWindow::EshyWMWindow(Window _window) : window(_window)
{
    position_data = new window_position_data();
}


void EshyWMWindow::frame_window(bool b_was_created_before_window_manager)
{
    if(!EshyWM::get_current_config()->frame_window)
    {
        return;
    }

    //Retrieve attributes of window to frame
    XWindowAttributes x_window_attributes = {0};
    CHECK(XGetWindowAttributes(EshyWM::get_window_manager()->get_display(), window, &x_window_attributes));

    //If window was created before window manager started, we should frame it only if it is visible and does not set override_redirect
    if(b_was_created_before_window_manager && (x_window_attributes.override_redirect || x_window_attributes.map_state != IsViewable))
    {
        return;
    }

    //const unsigned int border_width = EshyWM::get_current_config()->window_frame_border_width;
    //const unsigned long border_color = EshyWM::get_current_config()->window_frame_border_color;
    //const unsigned long background_color = EshyWM::get_current_config()->window_background_color;

    const unsigned int window_bar_height = 20;

    const unsigned int border_width = 2;
    const unsigned long border_color = 0x2b2a38;
    const unsigned long background_color = 0x15141f;

    //Create frame
    frame = XCreateSimpleWindow(EshyWM::get_window_manager()->get_display(), EshyWM::get_window_manager()->get_root(), x_window_attributes.x, x_window_attributes.y, x_window_attributes.width, x_window_attributes.height + window_bar_height, border_width, border_color, background_color);
    XSelectInput(EshyWM::get_window_manager()->get_display(), frame, SubstructureRedirectMask | SubstructureNotifyMask);
    XAddToSaveSet(EshyWM::get_window_manager()->get_display(), window);
    XReparentWindow(EshyWM::get_window_manager()->get_display(), window, frame, 0, window_bar_height);
    XMapWindow(EshyWM::get_window_manager()->get_display(), frame);

    //Map events
    //Raise windows with left button
    XGrabButton(EshyWM::get_window_manager()->get_display(), Button1, 0, frame, false, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
    //Move windows with alt + left button
    XGrabButton(EshyWM::get_window_manager()->get_display(), Button1, 0, frame, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    //Resize windows with alt + right button
    XGrabButton(EshyWM::get_window_manager()->get_display(), Button3, 0, frame, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
}

void EshyWMWindow::unframe_window()
{
    if(frame)
    {
        //Reverse what we did in frame_window
        //Unmap frame
        XUnmapWindow(EshyWM::get_window_manager()->get_display(), frame);
        //Reparent client window back to root window. Last params are the offset of the client window within root
        XReparentWindow(EshyWM::get_window_manager()->get_display(), window, EshyWM::get_window_manager()->get_root(), 0, 0);
        //Remove client window from save set, as it is now unrelated to us.
        XRemoveFromSaveSet(EshyWM::get_window_manager()->get_display(), window);
        //Destroy frame
        XDestroyWindow(EshyWM::get_window_manager()->get_display(), frame);

        LOG(INFO) << "Unframed window " << window << " [" << frame << "]";
    }
}

void EshyWMWindow::setup_grab_events(bool b_was_created_before_window_manager)
{
    //Retrieve attributes of window to frame
    XWindowAttributes x_window_attributes = {0};
    CHECK(XGetWindowAttributes(EshyWM::get_window_manager()->get_display(), window, &x_window_attributes));

    //If window was created before window manager started, we should frame it only if it is visible and does not set override_redirect
    if(b_was_created_before_window_manager && (x_window_attributes.override_redirect || x_window_attributes.map_state != IsViewable))
    {
        return;
    }

    //Grab events for window management actions on client window
    //Raise windows with left button
    XGrabButton(EshyWM::get_window_manager()->get_display(), Button1, 0, window, false, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    //Move windows with alt + left button
    XGrabButton(EshyWM::get_window_manager()->get_display(), Button1, Mod1Mask, window, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    //Resize windows with alt + right button
    XGrabButton(EshyWM::get_window_manager()->get_display(), Button3, Mod1Mask, window, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    //Kill windows with alt + shift + c
    XGrabKey(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_C), Mod1Mask, window, false, GrabModeAsync, GrabModeAsync);
    //Switch windows with alt + tab
    XGrabKey(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Tab), Mod1Mask, window, false, GrabModeAsync, GrabModeAsync);

    //Resize windows with alt + arrow keys
    XGrabKey(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Left), Mod1Mask, window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Right), Mod1Mask, window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Up), Mod1Mask, window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Down), Mod1Mask, window, false, GrabModeAsync, GrabModeAsync);
}


void EshyWMWindow::resize_window_horizontal_left_arrow()
{
    window_size_data window_size = get_window_size();

    if(position_data->get_horizontal_slot() == 0)
    {
        window_size.width -= get_resize_step_horizontal();
    }
    else if (position_data->get_horizontal_slot() != 0 && position_data->get_horizontal_slot() != EshyWM::get_window_manager()->get_num_horizontal_slots() - 1)
    {
        window_size.width -= get_resize_step_horizontal() * 2;
        window_size.x += get_resize_step_horizontal();
    }
    else if(position_data->get_horizontal_slot() == EshyWM::get_window_manager()->get_num_horizontal_slots() - 1)
    {
        window_size.width += get_resize_step_horizontal();
        window_size.x -= get_resize_step_horizontal();
    }

    XMoveResizeWindow(EshyWM::get_window_manager()->get_display(), window, window_size.x, window_size.y, window_size.width, window_size.height);
}

void EshyWMWindow::resize_window_horizontal_right_arrow()
{
    window_size_data window_size = get_window_size();

    if(position_data->get_horizontal_slot() == 0)
    {
        window_size.width += get_resize_step_horizontal();
    }
    else if (position_data->get_horizontal_slot() != 0 && position_data->get_horizontal_slot() != EshyWM::get_window_manager()->get_num_horizontal_slots() - 1)
    {
        window_size.width += get_resize_step_horizontal() * 2;
        window_size.x -= get_resize_step_horizontal();
    }
    else if(position_data->get_horizontal_slot() == EshyWM::get_window_manager()->get_num_horizontal_slots() - 1)
    {
        window_size.width -= get_resize_step_horizontal();
        window_size.x += get_resize_step_horizontal();
    }

    XMoveResizeWindow(EshyWM::get_window_manager()->get_display(), window, window_size.x, window_size.y, window_size.width, window_size.height);
}

void EshyWMWindow::resize_window_vertical_up_arrow()
{
    window_size_data window_size = get_window_size();

    if(position_data->get_vertical_slot() == 0)
    {
        window_size.height -= get_resize_step_vertical();
    }
    else if (position_data->get_vertical_slot() != 0 && position_data->get_vertical_slot() != EshyWM::get_window_manager()->get_num_vertical_slots() - 1)
    {
        window_size.height += get_resize_step_vertical() * 2;
        window_size.y -= get_resize_step_vertical();
    }
    else if(position_data->get_vertical_slot() == EshyWM::get_window_manager()->get_num_vertical_slots() - 1)
    {
        window_size.height += get_resize_step_vertical();
        window_size.y -= get_resize_step_vertical();
    }

    XMoveResizeWindow(EshyWM::get_window_manager()->get_display(), window, window_size.x, window_size.y, window_size.width, window_size.height);
}

void EshyWMWindow::resize_window_vertical_down_arrow()
{
    window_size_data window_size = get_window_size();

    if(position_data->get_vertical_slot() == 0)
    {
        window_size.height += get_resize_step_vertical();
    }
    else if (position_data->get_vertical_slot() != 0 && position_data->get_vertical_slot() != EshyWM::get_window_manager()->get_num_vertical_slots() - 1)
    {
        window_size.height -= get_resize_step_vertical() * 2;
        window_size.y += get_resize_step_vertical();
    }
    else if(position_data->get_vertical_slot() == EshyWM::get_window_manager()->get_num_vertical_slots() - 1)
    {
        window_size.height -= get_resize_step_vertical();
        window_size.y += get_resize_step_vertical();
    }
    
    XMoveResizeWindow(EshyWM::get_window_manager()->get_display(), window, window_size.x, window_size.y, window_size.width, window_size.height);
}


window_size_data EshyWMWindow::get_window_size()
{
    Window return_window;
    int x;
    int y;
    unsigned int unsigned_null;
    unsigned int width;
    unsigned int height;

    XGetGeometry(EshyWM::get_window_manager()->get_display(), window, &return_window, &x, &y, &width, &height, &unsigned_null, &unsigned_null);
    return window_size_data(x, y, width, height);
}

int EshyWMWindow::get_resize_step_horizontal()
{
    return EshyWM::get_current_config()->resize_step_size_width;
}

int EshyWMWindow::get_resize_step_vertical()
{
    return EshyWM::get_current_config()->resize_step_size_height;
}