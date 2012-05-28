--------------------------------------------------------------------------------
                                 MICROPOLIS
               A micropolygon rasterizer (c) Thomas Weber 2012
--------------------------------------------------------------------------------


Micropolis is a micropolygon rasterizer implemented in OpenCL. 

It uses the REYES[1] algorithm to rasterize curved surfaces. This is
done by splitting the surface into sub-pixel sized polygons (micropolygons) 
and rasterizing them. This allows the rendering of highly detailed, displaced 
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
GLFW 2.7
Boost
OpenCL
DevIL
Python 2.7
Cheetah template engine 

To build call:

> git submodule init
> git submodule update
> scons

The SConstruct file is configured to look in AMDAPPROOTSDK for the OpenCL
headers. If you're using another OpenCL vendor, you should set the
include dir accordingly.

The build-system uses code-generation for creating the config-file
parser and the OpenGL extension loader. This is done by two python
programs(configGen and flextGLgen) in the tools directory. They reside
in separate git submodules, so it is necessary to initialize and
update them before compiling with SCons.

To build on Windows you need:

Visual Studio 2010
OpenCL
Python 2.7
Cheetah template engine
Boost (root at $BOOSTROOT environment variable)

Update the submodules and call generate_source.bat to generate the
config-loader and extension loaders.

Open micropolis.vcxproj and compile.

Like with the linux build, the Visual Studio project is set to look in
$AMDAPPSDKROOT for OpenCL headers and libraries. Change if necessary.


--- USAGE ---


Just call:

>./micropolis

on linux or press F5 to start in Visual Studio.

You will see performance statistics on the command line.

Q or ESC close the application
UP and DOWN move the object along the Z axis
F3 shows the object in wireframe mode after bound&split

You can configure the program by modifying the options.txt
configuration file. The file is pretty well-documented.

Interesting configuration-values are:

input_file:
    The model to render.
    Look in the data/ dir for model files. The default teapot model
    is the most interesting. You may have to set flip_surface=true if
    models appear to be rendered inside-out

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
    pixel-sized micropolygons. More is a waste. Use powers of two (min=16).
    

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

Solving these issues will be part of my master's thesis.

Further limitations are

- Surface shader and displacement are hard-coded in the OpenCL kernel
- Only a Bezier surface meshes are supported
- Only one object can be rendered at a time and only moved along the Z axis


--- REFERENCES ---


[1] "The Reyes image rendering architecture", 
    H.L. Cook, L. Carpenter, E. Catmull
    Coputer Science Press, Inc. New York, NY, USA, 1988


--- COPYRIGHT & LICENSE ---


Copyright Thomas Weber 2011-2012

Micropolis is licensed under the GPLv3.
