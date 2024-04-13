
#include "config.h"
#include "X11.h"
#include "util.h"

#include <X11/Xatom.h>

#include <assert.h>
#include <string.h>
#include <ranges>
#include <algorithm>

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


RRMonitorInfo::RRMonitorInfo(const RRMonitorInfo& other)
{
    XRRMonitorInfo* monitors_arr = new XRRMonitorInfo[monitors.size()];
    for(int i = 0; i < monitors.size(); ++i)
    {
        monitors_arr[i] = other.monitors[i];
    }
    monitors = std::span(monitors_arr, monitors.size());
}

RRMonitorInfo::RRMonitorInfo(RRMonitorInfo&& other)
{
    monitors = other.monitors;
    other.monitors = std::span<XRRMonitorInfo>();
}

RRMonitorInfo::~RRMonitorInfo()
{
    XFree(monitors.data());
}


const RRMonitorInfo get_monitors()
{
    assert(display);
    int n_monitors;
    XRRMonitorInfo* found_monitors = XRRGetMonitors(display, DefaultRootWindow(display), false, &n_monitors);
    RRMonitorInfo monitor_info;
    monitor_info.monitors = std::span<XRRMonitorInfo>(found_monitors, n_monitors);
    return std::move(monitor_info);
}


const std::string get_atom_name(Atom name)
{
    assert(display);
    return XGetAtomName(display, name);
}


void set_display(Display* _display)
{
    display = _display;
}

Display* get_display()
{
    assert(display);
    return display;
}

Window get_root_window()
{
    assert(display);
    return DefaultRootWindow(display);
}


const bool grab_server()
{
    assert(display);
    return XGrabServer(display) == Success;
}

const bool ungrab_server()
{
    assert(display);
    return XUngrabServer(display) == Success;
}


const bool allow_events(int event_mode, Time time)
{
    assert(display);
    return XAllowEvents(display, event_mode, time) == Success;
}


const Pos get_cursor_position()
{
    assert(display);
    Pos position;
	Window window_return;
    int others;
    uint mask_return;
    XQueryPointer(display, DefaultRootWindow(display), &window_return, &window_return, &position.x, &position.y, &others, &others, &mask_return);
	return position;
}


void grab_key(KeySym key_sym, unsigned int main_modifier, Window window)
{
    assert(display);
    XGrabKey(display, XKeysymToKeycode(display, key_sym), main_modifier | Mod2Mask, window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, key_sym), main_modifier, window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, key_sym), main_modifier | LockMask, window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, key_sym), main_modifier | Mod2Mask | LockMask, window, false, GrabModeAsync, GrabModeAsync);
}

void ungrab_key(KeySym key_sym, unsigned int main_modifier, Window window)
{
    assert(display);
    XUngrabKey(display, XKeysymToKeycode(display, key_sym), main_modifier, window);
    XUngrabKey(display, XKeysymToKeycode(display, key_sym), main_modifier | Mod2Mask, window);
    XUngrabKey(display, XKeysymToKeycode(display, key_sym), main_modifier | LockMask, window);
    XUngrabKey(display, XKeysymToKeycode(display, key_sym), main_modifier | Mod2Mask | LockMask, window);
}

void grab_button(int button, unsigned int main_modifier, Window window, unsigned int masks)
{
    assert(display);
    XGrabButton(display, button, main_modifier, window, false, masks, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(display, button, main_modifier | Mod2Mask, window, false, masks, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(display, button, main_modifier | LockMask, window, false, masks, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(display, button, main_modifier | Mod2Mask | LockMask, window, false, masks, GrabModeAsync, GrabModeAsync, None, None);
}

void ungrab_button(int button, unsigned int main_modifier, Window window)
{
    assert(display);
    XUngrabButton(display, button, main_modifier, window);
    XUngrabButton(display, button, main_modifier | Mod2Mask, window);
    XUngrabButton(display, button, main_modifier | LockMask, window);
    XUngrabButton(display, button, main_modifier | Mod2Mask | LockMask, window);
}


const WindowAttributes get_window_attributes(Window window)
{
    assert(display);
    XWindowAttributes attr = { 0 };
    XGetWindowAttributes(display, window, &attr);
    return {attr.x, attr.y, (uint)attr.width, (uint)attr.height, attr.map_state, (bool)attr.override_redirect};
}

const WindowProperty get_window_property(Window window, Atom property)
{
    assert(display);
	WindowProperty window_property;
    window_property.status = XGetWindowProperty(display, window, property, 0, 1024, False, AnyPropertyType, &window_property.type, &window_property.format, &window_property.n_items, &window_property.bytes_after, &window_property.property_value);
	return std::move(window_property);
}

const bool change_window_property(Window window, Atom property, Atom type, const int size, const unsigned char* new_property)
{
    assert(display);
    return XChangeProperty(display, window, atoms.window_class, type, size, PropModeReplace, new_property, strlen((const char*)new_property)) == Success;
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


const Window create_window(const Rect& geometry, long input_masks, int border_width)
{
    assert(display);
    Window window = XCreateSimpleWindow(display, DefaultRootWindow(display), geometry.x, geometry.y, geometry.width, geometry.height, border_width, EshyWMConfig::window_frame_border_color, EshyWMConfig::window_background_color);
    set_input_masks(window, input_masks);
    return window;
}

const bool set_border_width(Window window, int border_width)
{
    assert(display);
    return XSetWindowBorderWidth(display, window, border_width) == Success;
}

const bool set_border_color(Window window, Color border_color)
{
    assert(display);
    return XSetWindowBorder(display, window, border_color) == Success;
}

const bool set_background_color(Window window, Color background_color)
{
    assert(display);
    return XSetWindowBackground(display, window, background_color) == Success;
}

const bool close_window(Window window)
{
    Atom* supported_protocols;
    int num_supported_protocols;
    const int status = XGetWMProtocols(display, window, &supported_protocols, &num_supported_protocols);

    const bool b_protocol_exists = std::ranges::contains(std::span(supported_protocols, num_supported_protocols), X11::atoms.wm_delete_window);

    if(b_protocol_exists)
    {
        XEvent message;
        memset(&message, 0, sizeof(message));
        message.xclient.type = ClientMessage;
        message.xclient.message_type = X11::atoms.wm_protocols;
        message.xclient.window = window;
        message.xclient.format = 32;
        message.xclient.data.l[0] = X11::atoms.wm_delete_window;
        XSendEvent(display, window, false, 0 , &message);
        return true;
    }

    return false;
}

const bool kill_window(Window window)
{
    return XKillClient(display, window) == Success;
}

const bool destroy_window(Window window)
{
    assert(display);
    unmap_window(window);
    return XDestroyWindow(display, window) == Success;
}

const bool set_input_masks(Window window, long input_masks)
{
    assert(display);
    return XSelectInput(display, window, input_masks) == Success;
}

const bool map_window(Window window)
{
    assert(display);
    return XMapWindow(display, window) == Success;
}

const bool unmap_window(Window window)
{
    assert(display);
    return XUnmapWindow(display, window) == Success;
}

const bool reparent_window(Window window, Window parent, const Pos& offset)
{
    assert(display);
    return XReparentWindow(display, window, parent, offset.x, offset.y) == Success;
}

const bool focus_window(Window window)
{
    assert(display);
    return XSetInputFocus(display, window, RevertToNone, CurrentTime) == Success;
}

const bool raise_window(Window window)
{
    assert(display);
    return XRaiseWindow(display, window) == Success;
}

const bool move_window(Window window, const Pos& pos)
{
    assert(display);
    return XMoveWindow(display, window, pos.x, pos.y) == Success;
}

const bool move_window(Window window, const Rect& pos)
{
    assert(display);
    return XMoveWindow(display, window, pos.x, pos.y) == Success;
}

const bool resize_window(Window window, const Size& size)
{
    assert(display);
    return XResizeWindow(display, window, size.width, size.height) == Success;
}

const bool resize_window(Window window, const Rect& size)
{
    assert(display);
    return XResizeWindow(display, window, size.width, size.height) == Success;
}
};
