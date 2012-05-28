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
 * along with Micropolis.  If not, see <http://www.gnu.org/licenses/>.            *
\******************************************************************************/


#ifndef IMAGE_H
#define IMAGE_H

#include "common.h"

/**
 * Manages image object on the CPU side.
 */
class Image
{
    protected:

    int _dimensions; /**< The number of dimensions */
    int _width, /**< Image width */
        _height, /**< Image height */
        _depth; /**< Image depth */
    void *_data; /**< Image data */

    GLenum _format; /**< Image format */
    GLenum _type; /**< Image data type */
    GLenum _internal_format; /**< Equivalent GL texture type */

    size_t _bpp; /**< Bytes per pixel. */

    static bool devil_initialized; /**< Flag for DevIL initialization.*/

    public:

    /**
     * Create uninitialized image object
     * @param dimensions Image dimensions
     * @param w Image width
     * @param h Image height
     * @param d Image depth
     * @param format Image format
     * @param type Image data type
     * @param bpp Bytes per pixel
     */
    Image (int dimensions, int w, int h, int d,
           GLenum format, GLenum type,
           size_t bpp);

    /**
     * Copy constructor.
     * Swaps data.
     * @param image Original
     */
    Image (Image& image);

    /**
     * Construct from image file.
     * @param filename Path to image file.
     */
    Image (const string& filename);

    virtual ~Image ();

    /**
     * Save contents of image to a file.
     * @param filename Path to destination image file.
     */
    void save_to_file(const string& filename);

    int dimensions() const { return _dimensions; } /**< Get dimensions*/

    int width() const { return _width; } /**< Get width */
    int height() const { return _height; } /**< Get height */
    int depth() const { return _depth; } /**< Get depth */

    void* data() { return _data; }  /**< Get data */
    const void* const_data() const { return _data; } /**< Get data as const */ 

    GLenum format() const { return _format; } /**< Get image format */
    GLenum type() const { return _type; } /**< Get image data type */
    /** Get internal format*/
    GLenum internal_format() const { return _internal_format; }
};

#endif

