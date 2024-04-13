
#include "util.h"

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include <span>

namespace X11
{
extern struct Atoms
{
    Atom cardinal;
	Atom supported;
	Atom active_window;
	Atom window_name;
	Atom window_class;
	Atom wm_protocols;
	Atom wm_delete_window;
	Atom window_type;
	Atom window_type_dock;
    Atom window_icon;
    Atom window_icon_name;
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

    explicit operator bool() const
    {
        return is_success();
    }

    bool is_success() const
    {
        return status == Success && type != None && format == 8;
    }
    
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

struct RRMonitorInfo
{
    //Technically RVO should make sure windows.data() is not freed when this is deleted
    //because this will not delete, but I have added move semantics to ensure that
    //regardless of compiler optimizations
    RRMonitorInfo() = default;
    RRMonitorInfo(const RRMonitorInfo& other);
    RRMonitorInfo(RRMonitorInfo&& other);
    ~RRMonitorInfo();
    
    std::span<XRRMonitorInfo> monitors;
};

extern const RRMonitorInfo get_monitors();

extern const std::string get_atom_name(Atom name);

extern void set_display(Display* display);
extern Display* get_display();
extern Window get_root_window();

extern const bool grab_server();
extern const bool ungrab_server();

extern const bool allow_events(int event_mode, Time time);

extern const Pos get_cursor_position();

extern void grab_key(KeySym key_sym, unsigned int main_modifier, Window window);
extern void ungrab_key(KeySym key_sym, unsigned int main_modifier, Window window);
extern void grab_button(int button, unsigned int main_modifier, Window window, unsigned int masks);
extern void ungrab_button(int button, unsigned int main_modifier, Window window);

extern const WindowAttributes get_window_attributes(Window window);
extern const WindowProperty get_window_property(Window window, Atom property);
extern const bool change_window_property(Window window, Atom property, Atom type, const int size, const unsigned char* new_property);

extern const WindowTree query_window_tree(Window window);

extern const Window create_window(const Rect& geometry, long input_masks, int border_width);
extern const bool set_border_width(Window window, int border_width);
extern const bool set_border_color(Window window, Color border_color);
extern const bool set_background_color(Window window, Color background_color);
extern const bool close_window(Window window);
extern const bool kill_window(Window window);
extern const bool destroy_window(Window window);
extern const bool set_input_masks(Window window, long input_masks);
extern const bool map_window(Window window);
extern const bool unmap_window(Window window);
extern const bool reparent_window(Window window, Window parent, const Pos& offset);
extern const bool focus_window(Window window);
extern const bool raise_window(Window window);
extern const bool move_window(Window window, const Pos& pos);
extern const bool move_window(Window window, const Rect& pos);
extern const bool resize_window(Window window, const Size& size);
extern const bool resize_window(Window window, const Rect& size);
};