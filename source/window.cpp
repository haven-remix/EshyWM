
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
    //Retrieve attributes of window to frame
    XWindowAttributes x_window_attributes = {0};
    CHECK(XGetWindowAttributes(EshyWM::get_window_manager()->get_display(), window, &x_window_attributes));

    //If window was created before window manager started, we should frame it only if it is visible and does not set override_redirect
    if(b_was_created_before_window_manager && (x_window_attributes.override_redirect || x_window_attributes.map_state != IsViewable))
    {
        return;
    }

    static Display* display = EshyWM::get_window_manager()->get_display();
    static const Window root = EshyWM::get_window_manager()->get_root();

    //Create frame and titlebar
    frame = XCreateSimpleWindow(display, root, x_window_attributes.x, x_window_attributes.y, x_window_attributes.width, x_window_attributes.height + title_bar_height, border_width, border_color, background_color);
    titlebar = XCreateSimpleWindow(display, root, x_window_attributes.x, x_window_attributes.y, x_window_attributes.width, title_bar_height, 0, border_color, background_color);
    XSelectInput(display, titlebar, SubstructureRedirectMask | SubstructureNotifyMask);
    XReparentWindow(display, window, frame, 0, title_bar_height);
    XReparentWindow(display, titlebar, frame, 0, 0);
    XMapWindow(display, frame);
    XMapWindow(display, titlebar);

    graphics_context_internal = XCreateGC(EshyWM::get_window_manager()->get_display(), frame, 0, 0);
    draw_titlebar_buttons();
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

    //Basic movement and resizing
    XGrabButton(EshyWM::get_window_manager()->get_display(), Button1, 0, frame, false, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    XGrabButton(EshyWM::get_window_manager()->get_display(), Button1, Mod1Mask, frame, false, ButtonPressMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(EshyWM::get_window_manager()->get_display(), Button3, Mod1Mask, frame, false, ButtonPressMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(EshyWM::get_window_manager()->get_display(), Button1, 0, titlebar, false, ButtonPressMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);

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


void EshyWMWindow::draw_titlebar_buttons()
{
    //Retrieve attributes of window to frame
    XWindowAttributes x_window_attributes = {0};
    XGetWindowAttributes(EshyWM::get_window_manager()->get_display(), window, &x_window_attributes);

    XClearWindow(EshyWM::get_window_manager()->get_display(), titlebar);
    XSetForeground(EshyWM::get_window_manager()->get_display(), graphics_context_internal, 0xffffff);
    XFillRectangle(EshyWM::get_window_manager()->get_display(), titlebar, graphics_context_internal, x_window_attributes.width - title_bar_button_size, 0, title_bar_button_size, title_bar_button_size);
    XFillRectangle(EshyWM::get_window_manager()->get_display(), titlebar, graphics_context_internal, (x_window_attributes.width - title_bar_button_size * 2) - title_bar_button_padding, 0, title_bar_button_size, title_bar_button_size);
    XFillRectangle(EshyWM::get_window_manager()->get_display(), titlebar, graphics_context_internal, (x_window_attributes.width - title_bar_button_size * 3) - (title_bar_button_padding * 2), 0, title_bar_button_size, title_bar_button_size);
}


int EshyWMWindow::is_cursor_on_titlebar_buttons(Window window, int cursor_x, int cursor_y)
{
    if(window != titlebar)
    {
        return 0;
    }

    //Retrieve attributes of window to frame
    XWindowAttributes x_window_attributes = {0};
    CHECK(XGetWindowAttributes(EshyWM::get_window_manager()->get_display(), window, &x_window_attributes));

    if(cursor_y > 0 && cursor_y < title_bar_height)
    {
        if(cursor_x < x_window_attributes.width && cursor_x > x_window_attributes.width - title_bar_button_size)
        {
            return 3;
        }
        else if(cursor_x < x_window_attributes.width - title_bar_button_size - title_bar_button_padding && cursor_x > x_window_attributes.width - (title_bar_button_size * 2) - title_bar_button_padding)
        {
            return 2;
        }
        else if(cursor_x < x_window_attributes.width - (title_bar_button_size * 2) - (title_bar_button_padding * 2) && cursor_x > x_window_attributes.width - (title_bar_button_size * 3) - (title_bar_button_padding * 2))
        {
            return 1;
        }
    }

    return 0;
}


void EshyWMWindow::resize_window_horizontal_left_arrow()
{
    // window_size_data window_size = get_window_size();

    // if(position_data->get_horizontal_slot() == 0)
    // {
    //     window_size.width -= get_resize_step_horizontal();
    // }
    // else if (position_data->get_horizontal_slot() != 0 && position_data->get_horizontal_slot() != EshyWM::get_window_manager()->get_num_horizontal_slots() - 1)
    // {
    //     window_size.width -= get_resize_step_horizontal() * 2;
    //     window_size.x += get_resize_step_horizontal();
    // }
    // else if(position_data->get_horizontal_slot() == EshyWM::get_window_manager()->get_num_horizontal_slots() - 1)
    // {
    //     window_size.width += get_resize_step_horizontal();
    //     window_size.x -= get_resize_step_horizontal();
    // }

    // XMoveResizeWindow(EshyWM::get_window_manager()->get_display(), window, window_size.x, window_size.y, window_size.width, window_size.height);
}

void EshyWMWindow::resize_window_horizontal_right_arrow()
{
    // window_size_data window_size = get_window_size();

    // if(position_data->get_horizontal_slot() == 0)
    // {
    //     window_size.width += get_resize_step_horizontal();
    // }
    // else if (position_data->get_horizontal_slot() != 0 && position_data->get_horizontal_slot() != EshyWM::get_window_manager()->get_num_horizontal_slots() - 1)
    // {
    //     window_size.width += get_resize_step_horizontal() * 2;
    //     window_size.x -= get_resize_step_horizontal();
    // }
    // else if(position_data->get_horizontal_slot() == EshyWM::get_window_manager()->get_num_horizontal_slots() - 1)
    // {
    //     window_size.width -= get_resize_step_horizontal();
    //     window_size.x += get_resize_step_horizontal();
    // }

    // XMoveResizeWindow(EshyWM::get_window_manager()->get_display(), window, window_size.x, window_size.y, window_size.width, window_size.height);
}

void EshyWMWindow::resize_window_vertical_up_arrow()
{
    // window_size_data window_size = get_window_size();

    // if(position_data->get_vertical_slot() == 0)
    // {
    //     window_size.height -= get_resize_step_vertical();
    // }
    // else if (position_data->get_vertical_slot() != 0 && position_data->get_vertical_slot() != EshyWM::get_window_manager()->get_num_vertical_slots() - 1)
    // {
    //     window_size.height += get_resize_step_vertical() * 2;
    //     window_size.y -= get_resize_step_vertical();
    // }
    // else if(position_data->get_vertical_slot() == EshyWM::get_window_manager()->get_num_vertical_slots() - 1)
    // {
    //     window_size.height += get_resize_step_vertical();
    //     window_size.y -= get_resize_step_vertical();
    // }

    // XMoveResizeWindow(EshyWM::get_window_manager()->get_display(), window, window_size.x, window_size.y, window_size.width, window_size.height);
}

void EshyWMWindow::resize_window_vertical_down_arrow()
{
    // window_size_data window_size = get_window_size();

    // if(position_data->get_vertical_slot() == 0)
    // {
    //     window_size.height += get_resize_step_vertical();
    // }
    // else if (position_data->get_vertical_slot() != 0 && position_data->get_vertical_slot() != EshyWM::get_window_manager()->get_num_vertical_slots() - 1)
    // {
    //     window_size.height -= get_resize_step_vertical() * 2;
    //     window_size.y += get_resize_step_vertical();
    // }
    // else if(position_data->get_vertical_slot() == EshyWM::get_window_manager()->get_num_vertical_slots() - 1)
    // {
    //     window_size.height -= get_resize_step_vertical();
    //     window_size.y += get_resize_step_vertical();
    // }
    
    // XMoveResizeWindow(EshyWM::get_window_manager()->get_display(), window, window_size.x, window_size.y, window_size.width, window_size.height);
}


void EshyWMWindow::resize_window(Vector2D<int> cursor_move_delta)
{
    const window_size_location_data data = get_frame_size_and_location_data();
    const Vector2D<int> size_delta(std::max(cursor_move_delta.x, -(int)data.width), std::max(cursor_move_delta.y, -(int)data.height));
    const size<int> final_frame_size = size<int>(data.width, data.height) + size_delta;

    XResizeWindow(EshyWM::get_window_manager()->get_display(), frame, final_frame_size.width, final_frame_size.height);
    XResizeWindow(EshyWM::get_window_manager()->get_display(), titlebar, final_frame_size.width, title_bar_height);
    XResizeWindow(EshyWM::get_window_manager()->get_display(), window, final_frame_size.width, final_frame_size.height - title_bar_height);
}

void EshyWMWindow::close_window()
{
    unframe_window();

    Atom* supported_protocols;
    int num_supported_protocols;

    if (XGetWMProtocols(EshyWM::get_window_manager()->get_display(), window, &supported_protocols, &num_supported_protocols) && (std::find(supported_protocols, supported_protocols + num_supported_protocols, EshyWM::get_window_manager()->get_atom_wm_delete_window()) != supported_protocols + num_supported_protocols))
    {
        LOG(INFO) << "Gracefully deleting window " << window;

        //Construct message
        XEvent message;
        memset(&message, 0, sizeof(message));
        message.xclient.type = ClientMessage;
        message.xclient.message_type = EshyWM::get_window_manager()->get_atom_wm_protocols();
        message.xclient.window = window;
        message.xclient.format = 32;
        message.xclient.data.l[0] = EshyWM::get_window_manager()->get_atom_wm_delete_window();

        //Send message to window to be closed
        CHECK(XSendEvent(EshyWM::get_window_manager()->get_display(), window, false, 0 , &message));
    }
    else
    {
        LOG(INFO) << "Killing window " << window;
        XKillClient(EshyWM::get_window_manager()->get_display(), window);
    }
}


void EshyWMWindow::recalculate_all_window_size_and_location()
{
    Window return_window;
    int x;
    int y;
    unsigned int unsigned_null;
    unsigned int width;
    unsigned int height;

    XGetGeometry(EshyWM::get_window_manager()->get_display(), frame, &return_window, &x, &y, &width, &height, &unsigned_null, &unsigned_null);
    frame_size_and_location_data = window_size_location_data(x, y, width, height);
    XGetGeometry(EshyWM::get_window_manager()->get_display(), titlebar, &return_window, &x, &y, &width, &height, &unsigned_null, &unsigned_null);
    titlebar_size_and_location_data = window_size_location_data(x, y, width, height);
    XGetGeometry(EshyWM::get_window_manager()->get_display(), window, &return_window, &x, &y, &width, &height, &unsigned_null, &unsigned_null);
    window_size_and_location_data = window_size_location_data(x, y, width, height);
}

int EshyWMWindow::get_resize_step_horizontal() const
{
    return EshyWM::get_current_config()->resize_step_size_width;
}

int EshyWMWindow::get_resize_step_vertical() const
{
    return EshyWM::get_current_config()->resize_step_size_height;
}