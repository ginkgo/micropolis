#ifndef REYES_H
#define REYES_H


size_t calc_grid_pos(size_t nu, size_t nv, size_t patch);

extern global float4* global pos_grid;
extern global int2* global pxlpos_grid;
extern global float* global depth_grid;
extern global float4* global color_grid;
extern global int4* global block_index;

#endif
