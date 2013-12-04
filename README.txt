--------------------------------------------------------------------------------
                                 MICROPOLIS
               A micropolygon rasterizer (c) Thomas Weber 2013
--------------------------------------------------------------------------------


Micropolis is a micropolygon rasterizer implemented in OpenCL. 

It uses the REYES[1] algorithm to rasterize curved surfaces. This is
done by splitting the surface into sub-pixel sized polygons (micropolygons) 
and rasterizing them. Doing so allows the rendering of highly detailed, displaced 
surfaces.

The dicing, shading and rasterization of the micropolygons is implemented in
OpenCL. The rasterizer fills a framebuffer that is then rendered as
texture in OpenGL. 
There also exists an alternative render backend that uses OpenGL hardware
tessellation for performance comparison.

Here is a video of it in action: 

http://www.youtube.com/watch?v=09ozb1ttgmA


--- COMPILATION ---


You will need the following dependencies to build micropolis on Linux:

SCons
GLFW 3.0
Boost
OpenCL
DevIL
Python 3.x
Wheezy template
Cap'n Proto

The source uses C++11 features, so you need a version of GCC recent enough
to support this.

To build call:

> git submodule init
> git submodule update
> scons

The build-system uses code-generation for creating the config-file
parser and the OpenGL extension loader. This is done by two python
programs(configGen and flextGLgen) in the tools directory. They reside
in separate git submodules, so it is necessary to initialize and
update them before compiling with SCons.


--- USAGE ---


Just call:

>./micropolis


You will see performance statistics on the command line.

Q or ESC close the application
Use WASD to move the camera
LMB+drag rotates the camera
MMB+drag moves the camera on the eye plane
RMB+drag rotates the camera along the z axis
PGUP/PGDOWN controls target patch size for Bound&Split algorithm
F3 toggles wireframe mode

You can configure the program by modifying the options.txt
configuration file. The file is pretty well-documented.

Interesting configuration-values are:

input_file:
    The scene to render.
    Look in the mscene/ dir for scene files.

renderer_type: 
    Switch between OpenCL rasterizer and hardware-tessellation backend

platform_id & device_id:
    The platform and device number of the OpenCL device that should be used.
     
disable_buffer_sharing:
    If your OpenCL device is not able to share buffers with the
    created OpenGL context (for instance because it is a CPU device),
    you might need to set this flag true. The framebuffer is then not
    shared and copying from OpenCL to OpenGL has to go through host
    memory. This takes extra time.

create_trace:
    Creates a reyes.trace trace file if true.
    This can then be used to visualize the timing of the individual
    render steps in the OpenCL rasterizer. You can do this by calling
    > tools/show_trace.sh reyes.trace
    This will create a PDF using pycairo and show it with Evince.

reyes_patches_per_pass:
    Sets how many sub-patches are diced and rasterized at once. Larger
    batches require more OpenCL device memory and take longer. Smaller
    batches have more overhead. You should set something between 256
    and 4096. Use powers of two.

bound_n_split_limit:
    In the first step of REYES, all patches are split to this screen size.

reyes_patch_size:
    The horizontal and vertical dicing rate of the split subpatches.
    Should ideally be the same as bound_n_split_limit to get
    pixel-sized micropolygons. More is a waste. Needs to be divisible by 8.
    

--- LIMITATIONS ---


Micropolis is very much a prototype. It is limited in several ways:

- Surface patches are diced at a fixed rate
    This can lead to wasteful overtessellation
- Bound&Split ignores displacement
    This is why there can be missing surface-patches at the screen border.
    They are culled erroneously.
- Surface cracks are not handled
    This can result in holes between surface patches.
- Micropolygons are always flat-shaded
    Gouraud shading would result in fewer artifacts and most production
    renderers use it, but I don't really know how to calculate the vertex-
    normal of displaced vertices in an elegant way. This is especially 
    problematic at the borders of a surface.
- No Multisampling or stochastic rasterization is supported
- Bound&Split happens on the CPU
    This was one of the major simplifications I did to get the project off
    the ground fast. By now, B&S and Host->Device transfer of the split 
    surfaces has become a major bottle-neck. Doing B&S on the device would
    have several advantages (f.i. occlusion culling using the depth-buffer 
    z-pyramid) but may need quite a lot of host-intervention and may have
    an unpredictable memory footprint.
    The OpenGL backend uses GPU-based B&S implemented in OpenGL compute
    shaders. I will port this to OpenCL soon.

Solving these issues will be part of my master's thesis.

Further limitations are

- Surface shader and displacement are hard-coded in the OpenCL kernel
- Only a Bezier surface meshes are supported
- Only one object can be rendered at a time and only moved along the Z axis


--- REFERENCES ---


[1] "The Reyes image rendering architecture", 
    H.L. Cook, L. Carpenter, E. Catmull
    Computer Science Press, Inc. New York, NY, USA, 1988


--- COPYRIGHT & LICENSE ---


Copyright Thomas Weber 2011-2013

Micropolis is licensed under the GPLv3.
