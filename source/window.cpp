
extern "C" {
#include <X11/Xutil.h>
}

#include "window_manager.h"
#include "window.h"
#include "config.h"
#include "eshywm.h"

#include <glog/logging.h>

#include <algorithm>


EshyWMWindow::EshyWMWindow(Window _window) : window(_window)
{
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
    frame = XCreateSimpleWindow(
        display,
        root,
        x_window_attributes.x,
        x_window_attributes.y,
        x_window_attributes.width,
        x_window_attributes.height + (EshyWM::get_window_manager()->get_manager_data()->b_tiling_mode ? 0 : EshyWM::get_current_config()->titlebar_height),
        EshyWM::get_current_config()->window_frame_border_width,
        EshyWM::get_current_config()->window_frame_border_color,
        EshyWM::get_current_config()->window_background_color
    );
    XReparentWindow(display, window, frame, 0, EshyWM::get_window_manager()->get_manager_data()->b_tiling_mode ? 0 : EshyWM::get_current_config()->titlebar_height);
    XMapWindow(display, frame);

    if(!EshyWM::get_window_manager()->get_manager_data()->b_tiling_mode)
    {
        titlebar = XCreateSimpleWindow(
            display,
            root,
            x_window_attributes.x,
            x_window_attributes.y,
            x_window_attributes.width,
            EshyWM::get_current_config()->titlebar_height,
            0,
            EshyWM::get_current_config()->window_frame_border_color,
            EshyWM::get_current_config()->window_background_color
        );
        XSelectInput(display, titlebar, SubstructureRedirectMask | SubstructureNotifyMask);
        XReparentWindow(display, titlebar, frame, 0, 0);
        XMapWindow(display, titlebar);
    }

    graphics_context_internal = XCreateGC(EshyWM::get_window_manager()->get_display(), frame, 0, 0);
    draw_titlebar_buttons();
}

void EshyWMWindow::unframe_window()
{
    if(frame)
    {
        XUnmapWindow(EshyWM::get_window_manager()->get_display(), frame);
        XReparentWindow(EshyWM::get_window_manager()->get_display(), window, EshyWM::get_window_manager()->get_root(), 0, 0);
        XRemoveFromSaveSet(EshyWM::get_window_manager()->get_display(), window);
        XDestroyWindow(EshyWM::get_window_manager()->get_display(), frame);
    }

    if(!EshyWM::get_window_manager()->get_manager_data()->b_tiling_mode && titlebar)
    {
        XUnmapWindow(EshyWM::get_window_manager()->get_display(), titlebar);
        XDestroyWindow(EshyWM::get_window_manager()->get_display(), titlebar);
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

    if(!EshyWM::get_window_manager()->get_manager_data()->b_tiling_mode)
    {
        XGrabButton(EshyWM::get_window_manager()->get_display(), Button1, 0, titlebar, false, ButtonPressMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
        XGrabButton(EshyWM::get_window_manager()->get_display(), Button3, 0, titlebar, false, ButtonPressMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    }

    //Kill windows with alt + shift + c
    XGrabKey(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_C), Mod1Mask, frame, false, GrabModeAsync, GrabModeAsync);
    //Switch windows with alt + tab
    XGrabKey(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Tab), Mod1Mask, frame, false, GrabModeAsync, GrabModeAsync);

    //Resize windows with alt + arrow keys
    XGrabKey(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Left), Mod1Mask, frame, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Right), Mod1Mask, frame, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Up), Mod1Mask, frame, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Down), Mod1Mask, frame, false, GrabModeAsync, GrabModeAsync);

    //Move windows with alt + shift + arrow keys
    XGrabKey(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Left), Mod1Mask | ControlMask, frame, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Right), Mod1Mask | ControlMask, frame, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Up), Mod1Mask | ControlMask, frame, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Down), Mod1Mask | ControlMask, frame, false, GrabModeAsync, GrabModeAsync);
}

void EshyWMWindow::remove_grab_events()
{
    //Basic movement and resizing
    XUngrabButton(EshyWM::get_window_manager()->get_display(), Button1, 0, frame);
    XUngrabButton(EshyWM::get_window_manager()->get_display(), Button1, Mod1Mask, frame);
    XUngrabButton(EshyWM::get_window_manager()->get_display(), Button3, Mod1Mask, frame);
    XUngrabButton(EshyWM::get_window_manager()->get_display(), Button1, 0, titlebar);
    XUngrabButton(EshyWM::get_window_manager()->get_display(), Button3, 0, titlebar);

    //Kill windows with alt + shift + c
    XUngrabButton(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_C), Mod1Mask, frame);
    //Switch windows with alt + tab
    XUngrabButton(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Tab), Mod1Mask, frame);

    //Resize windows with alt + arrow keys
    XUngrabButton(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Left), Mod1Mask, frame);
    XUngrabButton(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Right), Mod1Mask, frame);
    XUngrabButton(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Up), Mod1Mask, frame);
    XUngrabButton(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Down), Mod1Mask, frame);

    //Move windows with alt + shift + arrow keys
    XUngrabButton(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Left), Mod1Mask | ControlMask, frame);
    XUngrabButton(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Right), Mod1Mask | ControlMask, frame);
    XUngrabButton(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Up), Mod1Mask | ControlMask, frame);
    XUngrabButton(EshyWM::get_window_manager()->get_display(), XKeysymToKeycode(EshyWM::get_window_manager()->get_display(), XK_Down), Mod1Mask | ControlMask, frame);
}

void EshyWMWindow::draw_titlebar_buttons()
{
    if(EshyWM::get_window_manager()->get_manager_data()->b_tiling_mode)
    {
        return;
    }

    //Retrieve attributes of window to frame
    XWindowAttributes x_window_attributes = {0};
    XGetWindowAttributes(EshyWM::get_window_manager()->get_display(), window, &x_window_attributes);

    XClearWindow(EshyWM::get_window_manager()->get_display(), titlebar);
    XSetForeground(EshyWM::get_window_manager()->get_display(), graphics_context_internal, EshyWM::get_current_config()->titlebar_button_color);
    XFillRectangle(
        EshyWM::get_window_manager()->get_display(),
        titlebar,
        graphics_context_internal,
        x_window_attributes.width - EshyWM::get_current_config()->titlebar_button_size,
        0,
        EshyWM::get_current_config()->titlebar_button_size,
        EshyWM::get_current_config()->titlebar_button_size
    );
    XFillRectangle(
        EshyWM::get_window_manager()->get_display(),
        titlebar,
        graphics_context_internal,
        (x_window_attributes.width - EshyWM::get_current_config()->titlebar_button_size * 2) - EshyWM::get_current_config()->titlebar_button_padding,
        0,
        EshyWM::get_current_config()->titlebar_button_size,
        EshyWM::get_current_config()->titlebar_button_size
    );
    XFillRectangle(
        EshyWM::get_window_manager()->get_display(),
        titlebar,
        graphics_context_internal,
        (x_window_attributes.width - EshyWM::get_current_config()->titlebar_button_size * 3) - (EshyWM::get_current_config()->titlebar_button_padding * 2),
        0,
        EshyWM::get_current_config()->titlebar_button_size,
        EshyWM::get_current_config()->titlebar_button_size
    );
}

void EshyWMWindow::close_window()
{
    unframe_window();
    remove_grab_events();

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


int EshyWMWindow::is_cursor_on_titlebar_buttons(Window window, int cursor_x, int cursor_y)
{
    if(EshyWM::get_window_manager()->get_manager_data()->b_tiling_mode || window != titlebar)
    {
        return 0;
    }

    //Retrieve attributes of window to frame
    XWindowAttributes x_window_attributes = {0};
    CHECK(XGetWindowAttributes(EshyWM::get_window_manager()->get_display(), window, &x_window_attributes));

    if(cursor_y > 0 && cursor_y < EshyWM::get_current_config()->titlebar_height)
    {
        if(cursor_x < x_window_attributes.width && cursor_x > x_window_attributes.width - EshyWM::get_current_config()->titlebar_button_size)
        {
            return 3;
        }
        else if(cursor_x < x_window_attributes.width - EshyWM::get_current_config()->titlebar_button_size - EshyWM::get_current_config()->titlebar_button_padding && cursor_x > x_window_attributes.width - (EshyWM::get_current_config()->titlebar_button_size * 2) - EshyWM::get_current_config()->titlebar_button_padding)
        {
            return 2;
        }
        else if(cursor_x < x_window_attributes.width - (EshyWM::get_current_config()->titlebar_button_size * 2) - (EshyWM::get_current_config()->titlebar_button_padding * 2) && cursor_x > x_window_attributes.width - (EshyWM::get_current_config()->titlebar_button_size * 3) - (EshyWM::get_current_config()->titlebar_button_padding * 2))
        {
            return 1;
        }
    }

    return 0;
}


void EshyWMWindow::resize_window(Vector2D<int> delta)
{
    const rect data = get_frame_size_and_location_data();
    const Vector2D<int> size_delta(std::max(delta.x, -(int)data.width), std::max(delta.y, -(int)data.height));
    const Vector2D<int> final_frame_size = Vector2D<int>(data.width, data.height) + size_delta;

    XResizeWindow(EshyWM::get_window_manager()->get_display(), frame, final_frame_size.x, final_frame_size.y);
    if(!EshyWM::get_window_manager()->get_manager_data()->b_tiling_mode)
    {
        XResizeWindow(EshyWM::get_window_manager()->get_display(), titlebar, final_frame_size.x, EshyWM::get_current_config()->titlebar_height);
    }
    XResizeWindow(EshyWM::get_window_manager()->get_display(), window, final_frame_size.x, final_frame_size.y - (EshyWM::get_window_manager()->get_manager_data()->b_tiling_mode ? 0 : EshyWM::get_current_config()->titlebar_height));
}

void EshyWMWindow::resize_window_absolute(Vector2D<int> new_size)
{
    XResizeWindow(EshyWM::get_window_manager()->get_display(), frame, new_size.x, new_size.y);
    if(!EshyWM::get_window_manager()->get_manager_data()->b_tiling_mode)
    {
        XResizeWindow(EshyWM::get_window_manager()->get_display(), titlebar, new_size.x, EshyWM::get_current_config()->titlebar_height);
    }
    XResizeWindow(EshyWM::get_window_manager()->get_display(), window, new_size.x, new_size.y - (EshyWM::get_window_manager()->get_manager_data()->b_tiling_mode ? 0 : EshyWM::get_current_config()->titlebar_height));
}

void EshyWMWindow::resize_window_horizontal_left_arrow()
{
    //set_preferred_size(Vector2D<int>(get_preferred_size().x + get_resize_step_horizontal(), get_preferred_size().y));
    //EshyWM::get_window_manager()->window_size_updated(this);
}

void EshyWMWindow::resize_window_horizontal_right_arrow()
{
    //set_preferred_size(Vector2D<int>(get_preferred_size().x - get_resize_step_horizontal(), get_preferred_size().y));
    //EshyWM::get_window_manager()->window_size_updated(this);
}

void EshyWMWindow::resize_window_vertical_up_arrow()
{

}

void EshyWMWindow::resize_window_vertical_down_arrow()
{

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
    frame_size_and_location_data = rect(x, y, width, height);
    if(!EshyWM::get_window_manager()->get_manager_data()->b_tiling_mode)
    {
        XGetGeometry(EshyWM::get_window_manager()->get_display(), titlebar, &return_window, &x, &y, &width, &height, &unsigned_null, &unsigned_null);
        titlebar_size_and_location_data = rect(x, y, width, height);
    }
    XGetGeometry(EshyWM::get_window_manager()->get_display(), window, &return_window, &x, &y, &width, &height, &unsigned_null, &unsigned_null);
    window_size_and_location_data = rect(x, y, width, height);
}

int EshyWMWindow::get_resize_step_horizontal() const
{
    return EshyWM::get_current_config()->resize_step_size_width;
}

int EshyWMWindow::get_resize_step_vertical() const
{
    return EshyWM::get_current_config()->resize_step_size_height;
}