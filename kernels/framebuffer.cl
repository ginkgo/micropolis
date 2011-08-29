
int calc_framebuffer_pos(int2 pxlpos, int bsize, int2 gridsize)
{
    int2 gridpos = pxlpos / bsize;
    int  grid_id = gridpos.x + gridsize.x * gridpos.y;
    int2 loclpos = pxlpos - gridpos * bsize;
    int  locl_id = loclpos.x + bsize * loclpos.y;

    return grid_id * bsize * bsize + locl_id;
}

__kernel void clear (__global float4* framebuffer, float4 color)
{
    int2  pxlpos   = (int2) {get_global_id(0),   get_global_id(1)};
    int   bsize    = get_local_size(0);
    int2  gridsize = (int2) {get_num_groups(0), get_num_groups(1)};
 
    int pos = calc_framebuffer_pos(pxlpos, bsize, gridsize);
    
    framebuffer[pos] = color;
}
