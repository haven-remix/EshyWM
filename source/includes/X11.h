
#include <X11/Xlib.h>

#include <span>

namespace X11
{
struct WindowProperty
{
	int status = 0;
    Atom type;
    int format = 0;
    unsigned long n_items = 0;
    unsigned long bytes_after = 0;
    unsigned char* property_value = nullptr;
};

struct WindowTree
{
    //INPORTANT: This status is 0when it fails
    int status = 0;
    Window root;
    Window parent;
    std::span<Window> windows;
};

extern void set_display(Display* display);
extern WindowProperty get_window_property(Window window, Atom property);
extern WindowTree query_window_tree(Window window);
};