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


/** @file Texture.h */

#ifndef TEXTURE_H
#define TEXTURE_H

#include "common.h"

class Image;

namespace GL {
    class Tex : boost::noncopyable
    {
        protected: 

        GLenum _bound_unit; /**< The current texture unit, 0 if unbound */
        GLenum _target; /**< The texture target */
        GLuint _texture_name; /**< The GL texture handle */

    
        Tex();
    
        public: 

        virtual ~Tex();

        /**
         * Bind texture.
         */
        void bind();

        /**
         * Unbind texture
         */
        void unbind();


        public:
          
        /**
         * Helper class for automatic texture-unit management.
         */
        class UnitManager
        {
            GLenum* _unit_list; /**< Stack with unused texture units. */
            GLint _available_units; /**< Number of available units on stack. */
            GLint _max_units; /**< Total size of stack. */

            public:

            GLenum get_unit(); /**< Acquire unused texture unit */
            void return_unit(GLenum unit); /**< Return texture unit */

            UnitManager() :
                _unit_list(NULL) {}
            ~UnitManager();

            private:

            /**
             * Call this function during Texture construction.
             */
            void initialize();
        };
    
        /**
         * Returns the texture unit number, the texture is bound to.
         */
        GLint get_unit_number () const { return _bound_unit - GL_TEXTURE0; }

        GLuint texture_name() { return _texture_name; }

        bool is_bound() const { return _bound_unit != 0; }


        private:

        /**
         * Texture unit manager
         */
        static UnitManager _unit_manager;
    };

    /**
     * Represents a texture in GPU memory.
     */
    class Texture : public Tex
    {
        GLenum _min_filter; /**< The used minification filter */
        GLenum _mag_filter; /**< The used magnification filter */
        GLenum _wrap_method; /**< The used wrap method. Used for all directions */

        int _dimensions; /**< Number of dimensions. 1, 2 or 3 */
        GLenum _format; /**< GL image format */
        GLenum _internal_format; /**< GL internal format */
        int _width, /**< image width */
            _height, /**< image height. 0 if 1D */
            _depth; /**< image depth. 0 if 1D or 2D */
        

        public:

        /**
         * Construct texture from image object.
         * @param image The source image.
         * @param mag_filter Magnification filter.
         * @param min_filter Minification filter.
         * @param wrap_method Texture wrap method.
         */
        Texture(const Image& image,
                GLenum mag_filter = GL_LINEAR,
                GLenum min_filter = GL_LINEAR_MIPMAP_LINEAR,
                GLenum wrap_method = GL_REPEAT);

        /**
         * Construct an uninitialized texture.
         * @param dimensions Number of dimensions.
         * @param w Image width.
         * @param h Image height. 0 if dimensions < 2.
         * @param d Image depth. 0 if dimensions < 3.
         * @param format Image format.
         * @param internal_format Internal texture format.
         * @param mag_filter Magnification filter.
         * @param min_filter Minification filter.
         * @param wrap_method Texture wrap method.
         */
        Texture(int dimensions, int w, int h, int d,
                GLenum format, GLenum internal_format,
                GLenum mag_filter = GL_LINEAR,
                GLenum min_filter = GL_LINEAR_MIPMAP_LINEAR,
                GLenum wrap_method = GL_REPEAT,
                int samples = 0); 
    
        //~Texture();

        /**
         * Copy contents of texture to Image object.
         * @param image Destination image.
         */
        void read_back(Image& image);

        void generate_mipmaps();

        int width() const { return _width; }
        int height() const { return _height; }
        int depth() const { return _depth; }
    
        private:

        /**
         * Set up texture.
         * @param data Image data.
         * @param type Image type. 
         */
        void setup(const void* data, GLenum type, int samples);

    };

    class TextureBuffer : public Tex
    {
        GLuint _buffer;
        GLuint _size;

        public:

        TextureBuffer(GLuint size, GLenum internal_format);
        ~TextureBuffer();

        GLuint get_buffer() const { return _buffer; };
        GLuint get_size() const { return _size; };

        void load(void* data);
    };

}
#endif
