
#include "config.h"
#include "X11.h"
#include "util.h"

#include <X11/Xatom.h>

#include <assert.h>
#include <string.h>

static Display* display = nullptr;

namespace X11
{
Atoms atoms;

WindowProperty::WindowProperty(const WindowProperty& other)
    : status(other.status)
    , type(other.type)
    , format(other.format)
    , bytes_after(other.bytes_after)
    , n_items(other.n_items)
{
    //property_value has multiple \0, one for each item. The new string we allocate needs to handle all of them.
    unsigned char* current_pointer = other.property_value;
    for(int i = 0; i < n_items; ++i)
    {
        current_pointer += strlen((const char*)other.property_value);
    }

    const size_t size = (current_pointer - other.property_value + 1) * sizeof(unsigned char);
    property_value = new unsigned char[size];
    memcpy(property_value, other.property_value, size);
}

WindowProperty::WindowProperty(WindowProperty&& other)
    : status(other.status)
    , type(other.type)
    , format(other.format)
    , bytes_after(other.bytes_after)
    , n_items(other.n_items)
{
	property_value = other.property_value;
    other.property_value = nullptr;
}

WindowProperty::~WindowProperty()
{
    XFree(property_value);
}


WindowTree::WindowTree(const WindowTree& other)
    : status(other.status)
    , root(other.root)
    , parent(other.parent)
{
    Window* windows_arr = new Window[windows.size()];
    for(int i = 0; i < windows.size(); ++i)
    {
        windows_arr[i] = other.windows[i];
    }
    windows = std::span(windows_arr, windows.size());
}

WindowTree::WindowTree(WindowTree&& other)
    : status(other.status)
    , root(other.root)
    , parent(other.parent)
{
    windows = other.windows;
    other.windows = std::span<Window>();
}

WindowTree::~WindowTree()
{
    XFree(windows.data());
}


void set_display(Display* _display)
{
    display = _display;
}

const WindowAttributes get_window_attributes(Window window)
{
    assert(display);
    XWindowAttributes attr = { 0 };
    XGetWindowAttributes(display, window, &attr);
    return {attr.x, attr.y, (uint)attr.width, (uint)attr.height, attr.map_state, attr.override_redirect};
}

const WindowProperty get_window_property(Window window, Atom property)
{
    assert(display);
	WindowProperty window_property;
    window_property.status = XGetWindowProperty(display, window, property, 0, 1024, False, AnyPropertyType, &window_property.type, &window_property.format, &window_property.n_items, &window_property.bytes_after, &window_property.property_value);
	return std::move(window_property);
}

const WindowTree query_window_tree(Window window)
{
    assert(display);
    WindowTree window_tree;
    Window* windows = nullptr;
    unsigned int n_windows = 0;
    window_tree.status = XQueryTree(display, window, &window_tree.root, &window_tree.parent, &windows, &n_windows);
    window_tree.windows = std::span<Window>(windows, n_windows);
    return std::move(window_tree);
}

const Window frame_window(Window window, const Rect& geometry, const Pos& offset, long input_masks, int border_width)
{
    assert(display);
    Window frame = create_window(geometry, input_masks, border_width);
    reparent_window(window, frame, offset);

    //In EshyWM, we set frame class to match the window to support compositors
    const WindowProperty class_property = X11::get_window_property(window, atoms.window_class);
    if(class_property.status == Success && class_property.type != None && class_property.format == 8)
    {
        const unsigned char* property = (unsigned char*)class_property.property_value;
        XChangeProperty(display, frame, atoms.window_class, XA_STRING, 8, PropModeReplace, property, strlen((const char*)property));
    }

    map_window(frame);
    return frame;
}

const Window create_window(const Rect& geometry, long input_masks, int border_width)
{
    assert(display);
    Window window = XCreateSimpleWindow(display, DefaultRootWindow(display), geometry.x, geometry.y, geometry.width, geometry.height, border_width, EshyWMConfig::window_frame_border_color, EshyWMConfig::window_background_color);
    set_input_masks(window, input_masks);
    return window;
}

void destroy_window(Window window)
{
    assert(display);
    //unmap_window(window);
    XDestroyWindow(display, window);
}

void set_input_masks(Window window, long input_masks)
{
    assert(display);
    XSelectInput(display, window, input_masks);
}

void map_window(Window window)
{
    assert(display);
    XMapWindow(display, window);
}

void unmap_window(Window window)
{
    assert(display);
    XUnmapWindow(display, window);
}

void reparent_window(Window window, Window parent, const Pos& offset)
{
    assert(display);
    XReparentWindow(display, window, parent, offset.x, offset.y);
}

void resize_window(Window window, const Size& size)
{
    assert(display);
    XResizeWindow(display, window, size.width, size.height);
}

void resize_window(Window window, const Rect& size)
{
    assert(display);
    XResizeWindow(display, window, size.width, size.height);
}
};
