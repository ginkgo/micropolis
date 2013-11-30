/******************************************************************************\
 * This file is part of Micropolis.                                           *
 *                                                                            *
 * Micropolis is free software: you can redistribute it and/or modify         *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * Micropolis is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with Micropolis.  If not, see <http://www.gnu.org/licenses/>.        *
\******************************************************************************/


#include "Image.h"

#include <IL/il.h>
#include "format_map.h"

bool Image::devil_initialized = false;

Image::Image(int dimensions, int w, int h, int d,
             GLenum format, GLenum type, size_t bpp) :
    _dimensions(dimensions),
    _width(w), _height(h), _depth(d),
    _format(format), _type(type), 
    _internal_format(get_internal_format(format, type)),
    _bpp(bpp)
{
    assert(dimensions >= 1 && dimensions <= 3);

    size_t size = w;
    if (dimensions >= 2) size *= h;
    if (dimensions >= 3) size *= d;

    _data = malloc(bpp * size);
}

Image::Image(Image& image) :
    _dimensions(image._dimensions),
    _width(image._width), _height(image._height), _depth(image._depth),
    _format(image._format), _type(image._type), 
    _internal_format(image._internal_format),
    _bpp(image._bpp)
{
    image._data = NULL;
}
    
#define PRINT_FIELD(field_name) cout << "\t" << #field_name << "=" << field_name << endl;

Image::Image(const string& filename) : 
    _data(NULL)
{

    if (!devil_initialized) {
        ilInit();
        ilEnable(IL_ORIGIN_SET);
        ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
        devil_initialized = true;
    }

    ILuint il_image;
    ilGenImages(1, &il_image);
    ilBindImage(il_image);

    ILboolean success = ilLoadImage((const ILstring)filename.c_str());

    if (!success) {
        cerr << "Image::Image "
             << "Failed to load image file " << filename << endl;
        
        _data = NULL;
        return;
    }

    _dimensions = 2;
    _format = ilGetInteger(IL_IMAGE_FORMAT);
    if (_format == IL_LUMINANCE) {
        _format = GL_RED;
        
    }

    _type = ilGetInteger(IL_IMAGE_TYPE);
    _width = ilGetInteger(IL_IMAGE_WIDTH);
    _height = ilGetInteger(IL_IMAGE_HEIGHT);
    _depth = 0;
    _bpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
    _data = malloc(_bpp * _width * _height);

    _internal_format = get_internal_format(_format, _type);

    ilCopyPixels(0, 0, 0, _width, _height, 1, 
                 ilGetInteger(IL_IMAGE_FORMAT), _type, _data);

    ilBindImage(0);
    ilDeleteImages(1, &il_image);

}

void Image::save_to_file(const string& filename)
{
    if (!devil_initialized) {
        ilInit();
        devil_initialized = true;
    }

    ILuint il_image;
    ilGenImages(1, &il_image);

    ilTexImage(_width, _height, 0, _bpp, _format, _type, _data);
    ILboolean success = ilSaveImage((const ILstring)filename.c_str());

    if (!success) {
        cerr << "Image::Image "
             << "Failed to save to image file " 
             << filename << endl;
        
        _data = NULL;
        return;
    }    

    ilBindImage(0);
    ilDeleteImages(1, &il_image);
    
}

Image::~Image()
{
    if (_data != NULL) {
        free(_data);
    }
}
