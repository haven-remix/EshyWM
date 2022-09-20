
#include "window.h"
#include "config.h"
#include "eshywm.h"

extern "C" {
#include <X11/Xutil.h>
}

#include <glog/logging.h>

void EshyWMWindow::frame_window(bool b_was_created_before_window_manager)
{
    if(!EshyWM::get_eshywm_config()->frame_window)
    {
        return;
    }

    //Retrieve attributes of window to frame
    XWindowAttributes x_window_attributes = {0};
    CHECK(XGetWindowAttributes(display, window, &x_window_attributes));

    //If window was created before window manager started, we should frame it only if it is visible and does not set override_redirect
    if(b_was_created_before_window_manager && (x_window_attributes.override_redirect || x_window_attributes.map_state != IsViewable))
    {
        return;
    }

    //Create frame
    frame = XCreateSimpleWindow(display, EshyWM::get_window_manager()->get_root(), x_window_attributes.x, x_window_attributes.y, x_window_attributes.width, x_window_attributes.height, EshyWM::get_eshywm_config()->window_frame_border_width, EshyWM::get_eshywm_config()->window_frame_border_color, EshyWM::get_eshywm_config()->window_background_color);
    XSelectInput(display, frame, SubstructureRedirectMask | SubstructureNotifyMask);
    XAddToSaveSet(display, window);
    XReparentWindow(display, window, frame, 0, 0);
    XMapWindow(display, frame);
}

void EshyWMWindow::unframe_window()
{
    if(frame)
    {
        //Reverse what we did in frame_window
        //Unmap frame
        XUnmapWindow(display, frame);
        //Reparent client window back to root window. Last params are the offset of the client window within root
        XReparentWindow(display, window, EshyWM::get_window_manager()->get_root(), 0, 0);
        //Remove client window from save set, as it is now unrelated to us.
        XRemoveFromSaveSet(display, window);
        //Destroy frame
        XDestroyWindow(display, frame);

        LOG(INFO) << "Unframed window " << window << " [" << frame << "]";
    }
}

void EshyWMWindow::setup_grab_events(bool b_was_created_before_window_manager)
{
    //Retrieve attributes of window to frame
    XWindowAttributes x_window_attributes = {0};
    CHECK(XGetWindowAttributes(display, window, &x_window_attributes));

    //If window was created before window manager started, we should frame it only if it is visible and does not set override_redirect
    if(b_was_created_before_window_manager && (x_window_attributes.override_redirect || x_window_attributes.map_state != IsViewable))
    {
        return;
    }

    //Grab events for window management actions on client window
    //Raise windows with left button
    XGrabButton(display, Button1, 0, window, false, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    //Move windows with alt + left button
    XGrabButton(display, Button1, Mod1Mask, window, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    //Resize windows with alt + right button
    XGrabButton(display, Button3, Mod1Mask, window, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    //Kill windows with alt + shift + c
    XGrabKey(display, XKeysymToKeycode(display, XK_C), Mod1Mask, window, false, GrabModeAsync, GrabModeAsync);
    //Switch windows with alt + tab
    XGrabKey(display, XKeysymToKeycode(display, XK_Tab), Mod1Mask, window, false, GrabModeAsync, GrabModeAsync);

    //Resize windows with alt + arrow keys
    XGrabKey(display, XKeysymToKeycode(display, XK_Left), Mod1Mask, window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XK_Right), Mod1Mask, window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XK_Up), Mod1Mask, window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XK_Down), Mod1Mask, window, false, GrabModeAsync, GrabModeAsync);
}


void EshyWMWindow::reisze_window_horizontal_left_arrow()
{
    window_data window_size = get_window_size();

    switch(WindowSlot)
    {
    case EScreenSlot::SS_Left:
        window_size.width -= get_resize_step_horizontal();
        break;
    case EScreenSlot::SS_Right:
        window_size.width += get_resize_step_horizontal();
        window_size.x -= get_resize_step_horizontal();
        break;
    };

    XMoveResizeWindow(display, window, window_size.x, window_size.y, window_size.width, window_size.height);
}

void EshyWMWindow::reisze_window_horizontal_right_arrow()
{
    window_data window_size = get_window_size();

    switch(WindowSlot)
    {
    case EScreenSlot::SS_Left:
        window_size.width += get_resize_step_horizontal();
        break;
    case EScreenSlot::SS_Right:
        window_size.width -= get_resize_step_horizontal();
        window_size.x += get_resize_step_horizontal();
        break;
    };

    XMoveResizeWindow(display, window, window_size.x, window_size.y, window_size.width, window_size.height);
}

void EshyWMWindow::reisze_window_vertical_up_arrow()
{
    window_data window_size = get_window_size();

    switch(WindowSlot)
    {
    case EScreenSlot::SS_Top:
        window_size.height -= get_resize_step_vertical();
        break;
    case EScreenSlot::SS_Bottom:
        window_size.height += get_resize_step_vertical();
        window_size.y -= get_resize_step_vertical();
        break;
    default:
        window_size.height -= get_resize_step_vertical();
        break;
    };

    XMoveResizeWindow(display, window, window_size.x, window_size.y, window_size.width, window_size.height);
}

void EshyWMWindow::reisze_window_vertical_down_arrow()
{
    window_data window_size = get_window_size();

    switch(WindowSlot)
    {
    case EScreenSlot::SS_Top:
        window_size.height += get_resize_step_vertical();
        break;
    case EScreenSlot::SS_Bottom:
        window_size.height -= get_resize_step_vertical();
        window_size.y += get_resize_step_vertical();
        break;
    default:
        window_size.height += get_resize_step_vertical();
        break;
    };

    XMoveResizeWindow(display, window, window_size.x, window_size.y, window_size.width, window_size.height);
}


window_data EshyWMWindow::get_window_size()
{
    Window return_window;
    int x;
    int y;
    unsigned int unsigned_null;
    unsigned int width;
    unsigned int height;

    XGetGeometry(display, window, &return_window, &x, &y, &width, &height, &unsigned_null, &unsigned_null);
    return window_data(x, y, width, height);
}

int EshyWMWindow::get_resize_step_horizontal()
{
    return EshyWM::get_eshywm_config()->resize_step_size_width;
}

int EshyWMWindow::get_resize_step_vertical()
{
    return EshyWM::get_eshywm_config()->resize_step_size_height;
}