
#include "image.h"

Image::Image(const std::string& path)
    : image(imlib_load_image(path.c_str()))
    , b_this_owns_image(true)
{
}

Image::Image(Imlib_Image _image)
    : image(_image)
    , b_this_owns_image(false)
{
}

Image::~Image()
{
    if (b_this_owns_image)
    {
        imlib_context_set_image(image);
        imlib_free_image();
    }
}

void Image::draw(Drawable drawable, int x, int y)
{
    if (!image)
        return;

    imlib_context_set_drawable(drawable);
    imlib_context_set_image(image);
    imlib_context_set_blend(1);
    imlib_image_set_has_alpha(1);
    imlib_render_image_on_drawable(x, y);
}

void Image::draw(Drawable drawable, int x, int y, int width, int height)
{
    if (!image)
        return;
    
    imlib_context_set_drawable(drawable);
    imlib_context_set_image(image);
    imlib_context_set_blend(1);
    imlib_image_set_has_alpha(1);
    imlib_render_image_on_drawable_at_size(x, y, width, height);
}
