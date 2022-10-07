#pragma once

extern "C" {
#include <X11/Xlib.h>
}

#include <ostream>
#include <string>

template <typename T>
struct size {
    T width;
    T height;

    size() = default;
    
    size(T w, T h)
      : width(w)
      , height(h)
    {}

    std::string to_string() const;
};

struct window_size_location_data {
    int x;
    int y;
	unsigned int width;
	unsigned int height;

    window_size_location_data() = default;
    
    window_size_location_data(int _x, int _y)
      : x(_x)
      , y(_y)
    {}

	window_size_location_data(int _x, int _y, unsigned int _width, unsigned int _height)
      : x(_x)
      , y(_y)
	  , width(_width)
	  , height(_height)
    {}
};

// Outputs a size<T> as a string to a std::ostream.
template <typename T>
std::ostream& operator << (std::ostream& out, const size<T>& size);

// Represents a 2D vector.
template <typename T>
struct Vector2D {
	T x, y;

	Vector2D() = default;
	Vector2D(T _x, T _y)
		: x(_x), y(_y) {
	}

	std::string to_string() const;
};

// Outputs a size<T> as a string to a std::ostream.
template <typename T>
std::ostream& operator << (std::ostream& out, const Vector2D<T>& pos);

// Position operators.
template <typename T>
Vector2D<T> operator - (const Vector2D<T>& a, const Vector2D<T>& b);
template <typename T>
Vector2D<T> operator + (const Vector2D<T>& a, const Vector2D<T> &v);

// size operators.
template <typename T>
Vector2D<T> operator - (const size<T>& a, const size<T>& b);
template <typename T>
size<T> operator + (const size<T>& a, const Vector2D<T> &v);
template <typename T>
size<T> operator + (const Vector2D<T> &v, const size<T>& a);
template <typename T>
size<T> operator - (const size<T>& a, const Vector2D<T> &v);

// Joins a container of elements into a single string, with elements separated
// by a delimiter. Any element can be used as long as an operator << on ostream
// is defined.
template <typename Container>
std::string Join(const Container& container, const std::string& delimiter);

// Joins a container of elements into a single string, with elements separated
// by a delimiter. The elements are converted to string using a converter
// function.
template <typename Container, typename Converter>
std::string Join(const Container& container, const std::string& delimiter, Converter converter);

// Returns a string representation of a built-in type that we already have
// ostream support for.
template <typename T>
std::string to_string(const T& x);

// Returns a string describing an X event for debugging purposes.
extern std::string to_string(const XEvent& e);

// Returns a string describing an X window configuration value mask.
extern std::string XConfigureWindowValueMaskToString(unsigned long value_mask);

// Returns the name of an X request code.
extern std::string XRequestCodeToString(unsigned char request_code);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                               IMPLEMENTATION                              *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include <algorithm>
#include <vector>
#include <sstream>

template <typename T>
std::string size<T>::to_string() const
{
	std::ostringstream out;
	out << width << 'x' << height;
	return out.str();
}

template <typename T>
std::ostream& operator << (std::ostream& out, const size<T>& size)
{
  	return out << size.to_string();
}

template <typename T>
std::string Vector2D<T>::to_string() const
{
	std::ostringstream out;
	out << "(" << x << ", " << y << ")";
	return out.str();
}

template <typename T>
std::ostream& operator << (std::ostream& out, const Vector2D<T>& size)
{
  	return out << size.to_string();
}

template <typename T>
Vector2D<T> operator - (const Vector2D<T>& a, const Vector2D<T>& b)
{
  	return Vector2D<T>(a.x - b.x, a.y - b.y);
}

template <typename T>
Vector2D<T> operator + (const Vector2D<T>& a, const Vector2D<T> &v)
{
  	return Vector2D<T>(a.x + v.x, a.y + v.y);
}

template <typename T>
Vector2D<T> operator - (const size<T>& a, const size<T>& b)
{
  	return Vector2D<T>(a.width - b.width, a.height - b.height);
}

template <typename T>
size<T> operator + (const size<T>& a, const Vector2D<T> &v)
{
  	return size<T>(a.width + v.x, a.height + v.y);
}

template <typename T>
size<T> operator + (const Vector2D<T> &v, const size<T>& a)
{
  	return size<T>(a.width + v.x, a.height + v.y);
}

template <typename T>
size<T> operator - (const size<T>& a, const Vector2D<T> &v)
{
  	return size<T>(a.width - v.x, a.height - v.y);
}

template <typename Container>
std::string Join(const Container& container, const std::string& delimiter)
{
	std::ostringstream out;

	for (auto i = container.cbegin(); i != container.cend(); ++i)
	{
		if (i != container.cbegin())
		{
			out << delimiter;
		}

		out << *i;
	}

	return out.str();
}

template <typename Container, typename Converter>
std::string Join(const Container& container, const std::string& delimiter, Converter converter)
{
	std::vector<std::string> converted_container(container.size());
	std::transform(container.cbegin(), container.cend(), converted_container.begin(), converter);
	return Join(converted_container, delimiter);
}

template <typename T>
std::string to_string(const T& x)
{
	std::ostringstream out;
	out << x;
	return out.str();
}