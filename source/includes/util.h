#pragma once

#include <X11/Xlib.h>
#include <iostream>
#include <sstream>
#include <cmath>
#include <memory>

//Force enable logging
#define __LOGGING_ENABLED

#define ensure(s)             if(!(s)) abort();
#define safe_ensure(s)        if(!(s)) return;

#ifdef __LOGGING_ENABLED
	enum LogSeverity : uint8_t
	{
		LS_Verbose,
		LS_Info,
		LS_Warning,
		LS_Error,
		LS_Fatal
	};

	extern LogSeverity __global_log_severity;

	void __set_global_log_severity(LogSeverity severity);
	void __log_event_info(LogSeverity severity, XEvent event);
	void __log_message(LogSeverity severity, const char* message, ...);
	template <typename T>
	void __log_vector(LogSeverity severity, void* vector);

	#define SET_GLOBAL_SEVERITY(severity)		__set_global_log_severity(severity);
	#define LOG(severity, message, ...)			__log_message(severity, message);
	#define LOG_EVENT_INFO(severity, event)		__log_event_info(severity, event);
	#define LOG_VECTOR(severity, vector)		std::cout << "(" << vector.x << ", " << vector.y << ")" << std::endl;
#else
#ifndef __LOGGING_ENABLED
	#define SET_GLOBAL_SEVERITY(severity) ;
	#define LOG(severity, message)
	#define LOG_EVENT_INFO(severity, event)
	#define LOG_VECTOR(severity, vector)
#endif
#endif

typedef unsigned long Color;

#define CENTER_W(monitor, w)       (int)(monitor->x + ((monitor->width - w) / 2.0f))
#define CENTER_H(monitor, h)       (int)(monitor->y + ((monitor->height - EshyWMConfig::taskbar_height) - h) / 2.0f)

struct Pos
{
    int x;
    int y;
};

struct Size
{
	uint width;
	uint height;
};

struct Rect
{
	union
	{
		struct
		{
			int x;
			int y;
		};

		Pos position;
	};

	union
	{
		struct
		{
			uint width;
			uint height;
		};

		Size size;
	};
};

extern bool majority_monitor(Rect window_geometry, std::shared_ptr<struct s_monitor_info>& monitor_info);
extern bool position_in_monitor(int x, int y, std::shared_ptr<struct s_monitor_info>& monitor_info);

extern void set_window_transparency(Window window, float transparency);
extern void increment_window_transparency(Window window, float transparency);
extern void decrement_window_transparency(Window window, float transparency);