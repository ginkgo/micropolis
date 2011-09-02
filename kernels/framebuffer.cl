
__kernel void clear (__global float4* framebuffer, float4 color)
{
    int pos = get_global_id(0);
    framebuffer[pos] = color;
}
