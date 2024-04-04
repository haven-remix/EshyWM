
#include "util.h"
#include "window_manager.h"
#include "eshywm.h"

#include <fstream>
#include <stdarg.h>
#include <string.h>

#ifdef __LOGGING_ENABLED

#define LOG_SEVERITY_CHECK(severity) 
#define LOG_FATAL_CHECK(severity)	 if(severity == LogSeverity::LS_Fatal) abort();

Atoms atoms;

LogSeverity __global_log_severity;

void __set_global_log_severity(LogSeverity severity)
{
    __global_log_severity = severity;
}

void __log_message(LogSeverity severity, const char* message, ...)
{
	if(severity < __global_log_severity)
		return;

	if (FILE* file = fopen("/home/eshy/log.txt", "a"))
	{
		char _message[4096];
		va_list ap;
		va_start(ap, message);
		vsprintf(_message, message, ap);
		va_end(ap);

		fprintf(file, _message);
		fprintf(file, "\n");
		fclose(file);
	}
	
	if(severity == LogSeverity::LS_Fatal)
		abort();
}

void __log_event_info(LogSeverity severity, XEvent event)
{
    if(severity < __global_log_severity)
		return;

	switch(event.type)
	{
	case DestroyNotify:
		std::cout << "DestroyNotify: {"
			<< "send_event: " << event.xdestroywindow.send_event << ", "
			<< "event: " << event.xdestroywindow.event << ", "
			<< "window: " << event.xdestroywindow.window << "}"
    	<< std::endl;
		break;
	case MapNotify:
		std::cout << "MapNotify: {"
			<< "send_event: " << event.xmap.send_event << ", "
			<< "event: " << event.xmap.event << ", "
			<< "window: " << event.xmap.window << ", "
			<< "override_redirect: " << event.xmap.override_redirect << "}"
		<< std::endl;
		break;
	case UnmapNotify: \
		std::cout << "UnmapNotify: {"
			<< "send_event: " << event.xunmap.send_event << ", "
			<< "event: " << event.xunmap.event << ", "
			<< "window: " << event.xunmap.window << ", "
			<< "from_configure: " << event.xunmap.from_configure << "}"
		<< std::endl;
		break;
	case MapRequest:
		std::cout << "MapRequest: {"
			<< "send_event: " << event.xmaprequest.send_event << ", "
			<< "parent: " << event.xmaprequest.parent << ", "
			<< "window: " << event.xmaprequest.window << "}"
		<< std::endl;
		break;
	case ConfigureNotify:
		std::cout << "ConfigureNotify: {"
			<< "send_event: " << event.xconfigure.send_event << ", "
			<< "event: " << event.xconfigure.event << ", "
			<< "window: " << event.xconfigure.window << ", "
			<< "x: " << event.xconfigure.x << ", "
			<< "y: " << event.xconfigure.x << ", "
			<< "width: " << event.xconfigure.width << ", "
			<< "height: " << event.xconfigure.height << ", "
			<< "border_width: " << event.xconfigure.border_width << ", "
			<< "above: " << event.xconfigure.above << ", "
			<< "override_redirect: " << event.xconfigure.override_redirect << "}"
		<< std::endl;
		break;
	case ConfigureRequest:
		std::cout << "ConfigureRequestEvent: {"
			<< "send_event: " << event.xconfigurerequest.send_event << ", "
			<< "parent: " << event.xconfigurerequest.parent << ", "
			<< "window: " << event.xconfigurerequest.window << ", "
			<< "x: " << event.xconfigurerequest.x << ", "
			<< "y: " << event.xconfigurerequest.x << ", "
			<< "width: " << event.xconfigurerequest.width << ", "
			<< "height: " << event.xconfigurerequest.height << ", "
			<< "border_width: " << event.xconfigurerequest.border_width << ", "
			<< "above: " << event.xconfigurerequest.above << ", "
			<< "detail: " << event.xconfigurerequest.detail << ", "
			<< "value_mask: " << event.xconfigurerequest.value_mask << "}"
		<< std::endl;
		break;
	case ButtonPress:
		std::cout << "ButtonPressEvent: {"
			<< "send_event: " << event.xbutton.send_event << ", "
			<< "root: " << event.xbutton.root << ", "
			<< "window: " << event.xbutton.window << ", "
			<< "subwindow: " << event.xbutton.subwindow << ", "
			<< "x: " << event.xbutton.x << ", "
			<< "y: " << event.xbutton.y << ", "
			<< "x_root: " << event.xbutton.x_root << ", "
			<< "y_root: " << event.xbutton.y_root << ", "
			<< "state: " << event.xbutton.state << ", "
			<< "button: " << event.xbutton.button << "}"
		<< std::endl;
		break;
	case ButtonRelease:
		std::cout << "ButtonReleaseEvent: {"
			<< "send_event: " << event.xbutton.send_event << ", "
			<< "root: " << event.xbutton.root << ", "
			<< "window: " << event.xbutton.window << ", "
			<< "subwindow: " << event.xbutton.subwindow << ", "
			<< "x: " << event.xbutton.x << ", "
			<< "y: " << event.xbutton.y << ", "
			<< "x_root: " << event.xbutton.x_root << ", "
			<< "y_root: " << event.xbutton.y_root << ", "
			<< "state: " << event.xbutton.state << ", "
			<< "button: " << event.xbutton.button << "}"
		<< std::endl;
		break;
	case EnterNotify:
		std::cout << "EnterNotify: {"
			<< "send_event: " << event.xcrossing.send_event << ", "
			<< "root: " << event.xcrossing.root << ", "
			<< "window: " << event.xcrossing.window << ", "
			<< "subwindow: " << event.xcrossing.subwindow << ", "
			<< "x: " << event.xcrossing.x << ", "
			<< "y: " << event.xcrossing.y << ", "
			<< "x_root: " << event.xcrossing.x_root << ", "
			<< "y_root: " << event.xcrossing.y_root << ", "
			<< "mode: " << event.xcrossing.mode << ", "
			<< "detail: " << event.xcrossing.detail << ", "
			<< "focus: " << event.xcrossing.focus << ", "
			<< "state: " << event.xcrossing.state << "}"
		<< std::endl;
		break;
	case LeaveNotify:
		std::cout << "LeaveNotify: {"
			<< "send_event: " << event.xcrossing.send_event << ", "
			<< "root: " << event.xcrossing.root << ", "
			<< "window: " << event.xcrossing.window << ", "
			<< "subwindow: " << event.xcrossing.subwindow << ", "
			<< "x: " << event.xcrossing.x << ", "
			<< "y: " << event.xcrossing.y << ", "
			<< "x_root: " << event.xcrossing.x_root << ", "
			<< "y_root: " << event.xcrossing.y_root << ", "
			<< "mode: " << event.xcrossing.mode << ", "
			<< "detail: " << event.xcrossing.detail << ", "
			<< "focus: " << event.xcrossing.focus << ", "
			<< "state: " << event.xcrossing.state << "}"
		<< std::endl;
	};
	
	if(severity == LogSeverity::LS_Fatal)
		abort();
}
#endif

XPropertyReturn get_xwindow_property(Display* display, Window window, Atom property)
{
	XPropertyReturn xproperty_return;
    xproperty_return.status = XGetWindowProperty(DISPLAY, window, property, 0, 1024, False, AnyPropertyType, &xproperty_return.type, &xproperty_return.format, &xproperty_return.n_items, &xproperty_return.bytes_after, &xproperty_return.property_value);
	return xproperty_return;
}


Pos get_cursor_position(Display* display, Window root)
{
	Pos position;
	Window window_return;
    int others;
    uint mask_return;
    XQueryPointer(DISPLAY, ROOT, &window_return, &window_return, &position.x, &position.y, &others, &others, &mask_return);
	return position;
}

int center_x(struct Output* output, int width)
{
	return output->geometry.x + ((output->geometry.width - width) / 2.0f);
}

int center_y(struct Output* output, int height)
{
	return output->geometry.y + ((output->geometry.height - height) / 2.0f);
}


const bool is_within_rect(int x, int y, const Rect& rect)
{
	return x > rect.x && x < rect.x + rect.width && y > rect.y && y < rect.y + rect.height;
}


Output* output_at_position(int x, int y)
{
	auto it = std::ranges::find_if(EshyWM::window_manager->outputs, [x, y](auto output){
		return x >= output->geometry.x && x <= output->geometry.x + output->geometry.width && y >= output->geometry.y && y <= output->geometry.y + output->geometry.height;
	});

	return it != EshyWM::window_manager->outputs.end() ? *it : nullptr;
}

Output* output_most_occupied(Rect geometry)
{
	auto it = std::ranges::find_if(EshyWM::window_manager->outputs, [&geometry](auto output){
		const int window_center = geometry.x + (geometry.width / 2);
		return window_center > output->geometry.x && window_center < output->geometry.x + output->geometry.width;
	});

	return it != EshyWM::window_manager->outputs.end() ? *it : nullptr;
}


void grab_key(int key, unsigned int main_modifier, Window window)
{
    XGrabKey(DISPLAY, key, main_modifier, window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(DISPLAY, key, main_modifier | Mod2Mask, window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(DISPLAY, key, main_modifier | LockMask, window, false, GrabModeAsync, GrabModeAsync);
    XGrabKey(DISPLAY, key, main_modifier | Mod2Mask | LockMask, window, false, GrabModeAsync, GrabModeAsync);
}

void ungrab_key(int key, unsigned int main_modifier, Window window)
{
    XUngrabKey(DISPLAY, key, main_modifier, window);
    XUngrabKey(DISPLAY, key, main_modifier | Mod2Mask, window);
    XUngrabKey(DISPLAY, key, main_modifier | LockMask, window);
    XUngrabKey(DISPLAY, key, main_modifier | Mod2Mask | LockMask, window);
}

void grab_button(int button, unsigned int main_modifier, Window window, unsigned int masks)
{
    XGrabButton(DISPLAY, button, main_modifier, window, false, masks, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(DISPLAY, button, main_modifier | Mod2Mask, window, false, masks, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(DISPLAY, button, main_modifier | LockMask, window, false, masks, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(DISPLAY, button, main_modifier | Mod2Mask | LockMask, window, false, masks, GrabModeAsync, GrabModeAsync, None, None);
}

void ungrab_button(int button, unsigned int main_modifier, Window window)
{
    XUngrabButton(DISPLAY, button, main_modifier, window);
    XUngrabButton(DISPLAY, button, main_modifier | Mod2Mask, window);
    XUngrabButton(DISPLAY, button, main_modifier | LockMask, window);
    XUngrabButton(DISPLAY, button, main_modifier | Mod2Mask | LockMask, window);
}
