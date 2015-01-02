#ifndef REYES_H
#define REYES_H

// Patch type enums
#define BEZIER 0
#define GREGORY 1

// Defined in reyes.cl:

extern global float4* global pos_grid;
extern global int2* global pxlpos_grid;
extern global float* global depth_grid;
extern global float4* global color_grid;
extern global int4* global block_index;

__kernel void setup_intermediate_buffers (global float4* pos_grid_buffer,
                                          global int2* pxlpos_grid_buffer,
                                          global float* depth_grid_buffer,
                                          global float4* color_grid_buffer,
                                          global int4* block_index_buffer);
__kernel void shade(float4 diffuse_color);
__kernel void sample(volatile global int* tile_locks,
                     volatile global float4* color_buffer,
                     volatile global int* depth_buffer);

// __kernel void draw_patches(const global float4* patch_buffer,
//                            const global int* pid_buffer,
//                            const global float2* min_buffer,
//                            const global float2* max_buffer,
//                            float16 modelview,
//                            float16 proj,
//                            float4 diffuse_color,
//                            volatile global int* tile_locks,
//                            volatile global float4* color_buffer,
//                            volatile global int* depth_buffer);

size_t calc_grid_pos(size_t nu, size_t nv, size_t patch);
int calc_framebuffer_pos(int2 pxlpos);
size_t calc_grid_pos(size_t nu, size_t nv, size_t patch);
int is_front_facing(const int2 *ps);
int is_empty(int2 min, int2 max);
int calc_block_pos(int u, int v, int range_id);
int calc_color_grid_pos(int u, int v, int range_id);
int calc_tile_id(int tx, int ty);
void recover_patch_pos(size_t block_id, size_t lx, size_t ly, size_t* u, size_t* v, size_t* patch);
int3 idot3 (int3 Ax, int3 Ay, int3 Bx, int3 By);
int4 idot4 (int4 Ax, int4 Ay, int4 Bx, int4 By);
int idot (int2 a, int2 b);
int inside_triangle(int3 Px, int3 Py, int2 tp, float3 dv, float* depth);

// Defined in dice.cl (renamed w. preprocessor):

__kernel void dice_gregory (const global float4* patch_buffer,
                            const global int* pid_buffer,
                            const global float2* min_buffer,
                            const global float2* max_buffer,
                            float16 modelview,
                            float16 proj);
__kernel void dice_bezier (const global float4* patch_buffer,
                           const global int* pid_buffer,
                           const global float2* min_buffer,
                           const global float2* max_buffer,
                           float16 modelview,
                           float16 proj);

#endif
