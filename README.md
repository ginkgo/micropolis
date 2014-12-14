# MICROPOLIS

Micropolis is a micropolygon render implemented in OpenCL. 

It uses the REYES[1] algorithm to rasterize curved surfaces. This is
done by splitting the surface into sub-pixel sized polygons (micropolygons) 
and rasterizing them. Doing so allows the rendering of highly detailed, displaced 
surfaces.

The subdivision, dicing, shading and rasterization of the micropolygons is implemented in
OpenCL. The rasterizer fills a framebuffer that is then rendered as
texture in OpenGL. 
There also exists an alternative render backend that uses OpenGL hardware
tessellation for performance comparison.

Here is a video of it in action: [Video](www.youtube.com/watch?v=09ozb1ttgmA)


## COMPILATION


You will need the following dependencies to build micropolis on Linux:

- SCons
- GLFW 3.0
- Boost
- OpenCL
- DevIL
- Python 3.x
- Wheezy template
- Cap'n Proto

The source uses some C++11 features, so you need a version of GCC recent enough
to support this.

To build call:

```
$ git submodule init
$ git submodule update
$ scons
```

The build-system uses code-generation for creating the config-file
parser and the OpenGL extension loader. This is done by two Python
programs(`configGen` and `flextGLgen`) in the tools directory. They reside
in separate git submodules, so it is necessary to initialize and
update them before compiling with SCons.


## USAGE


Just call:

```
$ ./micropolis
```

You will see performance statistics on the command line.

- `Q` or `ESC` close the application
- Use `WASD` to move the camera
- `LMB`+drag rotates the camera
- `MMB`+drag moves the camera on the eye plane
- `RMB`+drag rotates the camera along the z axis
- `PGUP`/`PGDOWN` controls target patch size for Bound&Split algorithm
- `F3` toggles wireframe mode
- `F9` dumps a trace file which can be visualized using tools/show_trace.sh
- `F12` saves the current camera position as an mscene file
- `PRINT` creates a screenshot

You can configure the program by modifying the *.options configuration files.
The files are pretty well-documented.

Interesting configuration-values are:

- `input_file`:
    The scene to render.
    Look in the mscene/ dir for scene files.

- `opencl_device_id`:
    A pair of numbers giving platform and device id of the OpenCL device that should be used.
     
- `reyes_patches_per_pass`:
    Sets how many sub-patches are diced and rasterized at once. Larger
    batches require more OpenCL device memory and take longer. Smaller
    batches have more overhead. You should set something between 256
    and 4096. Use powers of two.

- `bound_n_split_method`:
    The method used for bound and split. Can be `BOUNDED`, `LOCAL`, `BREADTH`, or `CPU`

- `bound_n_split_limit`:
    In the first step of REYES, all patches are split to this screen size.

- `reyes_patch_size`:
    The horizontal and vertical dicing rate of the split subpatches.
    Should ideally be the same as `bound_n_split_limit` to get
    pixel-sized micropolygons. More is a waste. Needs to be divisible by 8.
    

## LIMITATIONS


Micropolis is very much a prototype. It is limited in several ways:

- **Surface patches are diced at a fixed rate**
    This can lead to wasteful overtessellation

- **Surface cracks are not handled**
    This can result in holes between surface patches.

- **Micropolygons are always flat-shaded**
    Gouraud shading would result in fewer artifacts.

- **No Multisampling or stochastic rasterization is supported**

- **Surface shader and displacement are hard-coded in the OpenCL kernel**

These problems may be fixed in the future. The first three issues will probably be solved by using DiagSplit and decoupled shading.


## REFERENCES

[1] "The Reyes image rendering architecture", 
    H.L. Cook, L. Carpenter, E. Catmull
    Computer Science Press, Inc. New York, NY, USA, 1988


## COPYRIGHT & LICENSE

Copyright Thomas Weber 2011-2014

Micropolis is licensed under the GPLv3.
