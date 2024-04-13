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
	#define LOGV(message, ...)					LOG(LogSeverity::LS_Verbose, message, ...)
	#define LOGI(message, ...)					LOG(LogSeverity::LS_Info, message, ...)
	#define LOGW(message, ...)					LOG(LogSeverity::LS_Warning, message, ...)
	#define LOGE(message, ...)					LOG(LogSeverity::LS_Error, message, ...)
	#define LOGF(message, ...)					LOG(LogSeverity::LS_Fatal, message, ...)
	#define LOG_EVENT_INFO(severity, event)		__log_event_info(severity, event);
	#define LOG_VECTOR(severity, vector)		std::cout << "(" << vector.x << ", " << vector.y << ")" << std::endl;
#else
#ifndef __LOGGING_ENABLED
	#define SET_GLOBAL_SEVERITY(severity) ;
	#define LOG(severity, message)
	#define LOGV(message, ...)
	#define LOGI(message, ...)
	#define LOGW(message, ...)
	#define LOGE(message, ...)
	#define LOGF(message, ...)
	#define LOG_EVENT_INFO(severity, event)
	#define LOG_VECTOR(severity, vector)
#endif
#endif

typedef unsigned long Color;

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

struct XPropertyReturn
{
	int status = 0;
    Atom type;
    int format = 0;
    unsigned long n_items = 0;
    unsigned long bytes_after = 0;
    unsigned char* property_value = nullptr;
};

extern struct Output* output_at_position(int x, int y);
extern struct Output* output_most_occupied(Rect geometry);

extern const bool is_within_rect(int x, int y, const Rect& rect);

//Returns x or y required to center a provided width and height in a output
extern int center_x(struct Output* output, int width);
extern int center_y(struct Output* output, int height);

extern void set_window_transparency(Window window, float transparency);
extern void increment_window_transparency(Window window, float transparency);
extern void decrement_window_transparency(Window window, float transparency);