
#include "util.h"

#include <X11/Xlib.h>

#include <span>

namespace X11
{
extern struct Atoms
{
	Atom supported;
	Atom active_window;
	Atom window_name;
	Atom window_class;
	Atom wm_protocols;
	Atom wm_delete_window;
	Atom window_type;
	Atom window_type_dock;
	Atom state;
	Atom state_fullscreen;
} atoms;

struct WindowAttributes
{
    union
    {
        Rect geometry;

        struct
        {
            int x;
            int y;
            uint width;
            uint height;
        };
    };
    int map_state;
    bool override_redirect;
};

struct WindowProperty
{
    //Technically RVO should make sure windows.data() is not freed when this is deleted
    //because this will not delete, but I have added move semantics to ensure that
    //regardless of compiler optimizations
    WindowProperty() = default;
    WindowProperty(const WindowProperty& other);
    WindowProperty(WindowProperty&& other);
    ~WindowProperty();
    
	int status = 0;
    Atom type;
    int format = 0;
    unsigned long bytes_after = 0;
    unsigned long n_items = 0;
    unsigned char* property_value = nullptr;
};

struct WindowTree
{
    //Technically RVO should make sure windows.data() is not freed when this is deleted
    //because this will not delete, but I have added move semantics to ensure that
    //regardless of compiler optimizations
    WindowTree() = default;
    WindowTree(const WindowTree& other);
    WindowTree(WindowTree&& other);
    ~WindowTree();
    
    //INPORTANT: This status is 0 when it fails
    int status = 0;
    Window root;
    Window parent;
    std::span<Window> windows;
};

extern void set_display(Display* display);
extern const WindowAttributes get_window_attributes(Window window);
extern const WindowProperty get_window_property(Window window, Atom property);
extern const WindowTree query_window_tree(Window window);
extern const Window frame_window(Window window, const Rect& geometry, const Pos& offset, long input_masks, int border_width);
extern const Window create_window(const Rect& geometry, long input_masks, int border_width);
extern void destroy_window(Window window);
extern void set_input_masks(Window window, long input_masks);
extern void map_window(Window window);
extern void unmap_window(Window window);
extern void reparent_window(Window window, Window parent, const Pos& offset);
extern void resize_window(Window window, const Size& size);
extern void resize_window(Window window, const Rect& size);
};