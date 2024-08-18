
#pragma once

#include <string>

#include <Imlib2.h>

class Image
{
public:

    Image() = default;
    Image(const std::string& path);
    Image(Imlib_Image _image);
    ~Image();

    void draw(Drawable drawable, int x, int y);
    void draw(Drawable drawable, int x, int y, int width, int height);

    Imlib_Image image;
    bool b_this_owns_image;
};