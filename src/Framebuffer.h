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


#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "common.h"

#include "Texture.h"
#include "Shader.h"
#include "OpenCL.h"
#include "VBO.h"

namespace Reyes
{
    
    class Framebuffer
    {
        
        protected:

        ivec2 _size;
        int   _tile_size;
        ivec2 _grid_size;
        ivec2 _act_size;

        GL::Shader _shader;
        CL::Program _framebuffer_program;
        scoped_ptr<CL::Kernel> _clear_kernel;


        CL::Buffer* _cl_buffer;
       
		GL::VBO _screen_quad;

        Framebuffer(CL::Device& device, const ivec2& size, int tile_size);


        public:

        virtual ~Framebuffer();


        CL::Event clear(CL::CommandQueue& queue, const CL::Event& e);


        const CL::Buffer& get_buffer() { return *_cl_buffer; }
        ivec2 size() {return _size; }
        int   get_tile_size() { return _tile_size; }
        ivec2 get_grid_size() { return _grid_size; }

        virtual CL::Event acquire(CL::CommandQueue& queue, const CL::Event& e) = 0;
        virtual CL::Event release(CL::CommandQueue& queue, const CL::Event& e) = 0;
        virtual void show() = 0;

    };


    class OGLSharedFramebuffer : public Framebuffer
    {

        GL::TextureBuffer _tex_buffer;
        bool _shared;
        void* _local;
		GLFWwindow* _glfw_window;
		

        public:

        OGLSharedFramebuffer(CL::Device& device, const ivec2& size, int tile_size, GLFWwindow* window);

        virtual ~OGLSharedFramebuffer();


        virtual CL::Event acquire(CL::CommandQueue& queue, const CL::Event& e);
        virtual CL::Event release(CL::CommandQueue& queue, const CL::Event& e);
                             

        void show();
    };
}

#endif
