/**
 * Copyright Haven_Remix 2022. All rights reserved.
*/

#include <X11/Xlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int main (void)
{
    Display* display;
    XWindowAttributes window_attributes;

    //Save pointer state at the beginning of the move/resize
    XButtonEvent start;
    XEvent event;

    //Return failure if we cannot connect
    if(!(display = XOpenDisplay(0x0))) return 1;

    XGrabKey(display, XKeysymToKeycode(display XStringToKeysym("F1")), Mod1Mask, DefaultRootWindow(display), True, GrabModeAsync);

    XGrabButton(display, 1, Mod1Mask, DefaultRootWindow(display), True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);    XGrabButton(display, 1, Mod1Mask, DefaultRootWindow(display), True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(display, 3, Mod1Mask, DefaultRootWindow(display), True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);

    start.subwindow = None;

    for(;;)
    {
        XNextEvent(display, &event);

        //Keybinding for raising windows
        if(event.type == KeyPress && event.xkey.subwindow != None)
        {
            XRaiseWindow(display, event.xkey.subwindow);
        }
        else if (event.type == ButtonPress && ev.xbutton.subwindow != None)
        {
            //Resizing windows
            XGetWindowAttributes(display, event.xbutton.subwindow, &window_attributes);
            start = event.xbutton;
        }
        else if(event.type == MotionNotify && start.subwindow != None)
        {
            int xdiff = ev.xbutton.x_root - start.x_root;
            int ydiff = ev.xbutton.y_root - start.y_root;
            XMoveResizeWindow(display, start.subwindow, window_attributes.x + (start.button==1 ? xdiff : 0), window_attributes.y + (start.button==1 ? ydiff : 0), MAX(1, window_attributes.width + (start.button==3 ? xdiff : 0)), MAX(1, window_attributes.height + (start.button==3 ? ydiff : 0)));
        }
        else if(event.type == ButtonRelease)
        {
            start.subwindow = None;
        }
    }
}