
#include "X11.h"

#include <assert.h>

static Display* display = nullptr;

namespace X11
{
void set_display(Display* _display)
{
    display = _display;
}

WindowProperty get_xwindow_property(Window window, Atom property)
{
    assert(display);
	WindowProperty window_property;
    window_property.status = XGetWindowProperty(display, window, property, 0, 1024, False, AnyPropertyType, &window_property.type, &window_property.format, &window_property.n_items, &window_property.bytes_after, &window_property.property_value);
	return window_property;
}

WindowTree query_window_tree(Window window)
{
    assert(display);
    WindowTree window_tree;
    Window* windows = nullptr;
    unsigned int n_windows = 0;
    XQueryTree(display, window, &window_tree.root, &window_tree.parent, &windows, &n_windows);
    window_tree.windows = std::span<Window>(windows, n_windows);
    return window_tree;
}
};