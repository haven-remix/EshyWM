
#include "background.h"
#include "X11.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <Imlib2.h>

namespace EshyBg
{
void set_background(const std::string& file_path)
{
    Imlib_Image bg_image = imlib_load_image(file_path.c_str());;
    if (!bg_image)
        return;

    const auto monitor_info = X11::get_monitors();
    if (monitor_info.monitors.size() == 0)
        return;

    const int width = monitor_info.monitors[0].width;
    const int height = monitor_info.monitors[0].height; 

    Display* display1 = X11::get_display();
    if (!display1)
        return;
    
    Window root1 = X11::get_root_window();
    const auto depth1 = DefaultDepth(display1, DefaultScreen(display1));

    const Pixmap bg_pixmap1 = XCreatePixmap(display1, root1, width, height, depth1);
    imlib_context_set_drawable(bg_pixmap1);
    imlib_context_set_image(bg_image);
    imlib_render_image_on_drawable_at_size(0, 0, width, height);
    imlib_free_image();

    //Open new display and copy pixmap over; done on a second display because nothing should have to interact with the wallpaper
    Display* display2 = XOpenDisplay(NULL);
    if (!display2)
        return;

    Window root2 = XRootWindow(display2, DefaultScreen(display2));
    const auto depth2 = DefaultDepth(display2, DefaultScreen(display2));
    XSync(display1, false);
    const Pixmap bg_pixmap2 = XCreatePixmap(display2, root2, width, height, depth2);

    XGCValues gc_values;
    gc_values.fill_style = FillTiled;
    gc_values.tile = bg_pixmap1;

    GC bg_gc = XCreateGC(display2, bg_pixmap2, GCFillStyle | GCTile, &gc_values);
    XFillRectangle(display2, bg_pixmap2, bg_gc, 0, 0, width, height);
    XFreeGC(display2, bg_gc);
    XSync(display2, false);
    XSync(display1, false);
    XFreePixmap(display1, bg_pixmap1);

	const Atom prop_root = XInternAtom(display2, "_XROOTPMAP_ID", False);
	const Atom prop_esetroot = XInternAtom(display2, "ESETROOT_PMAP_ID", False);

	if (prop_root == None || prop_esetroot == None)
        return;

	XChangeProperty(display2, root2, prop_root, XA_PIXMAP, 32, PropModeReplace, (unsigned char*)&bg_pixmap2, 1);
	XChangeProperty(display2, root2, prop_esetroot, XA_PIXMAP, 32, PropModeReplace, (unsigned char*)&bg_pixmap2, 1);

    XSetWindowBackgroundPixmap(display2, root2, bg_pixmap2);
    XClearWindow(display2, root2);

    XSetCloseDownMode(display2, RetainPermanent);
    XCloseDisplay(display2);
}
}