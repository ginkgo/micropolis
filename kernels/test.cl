
__kernel void test (write_only image2d_t framebuffer, float t)
{
    int2   coord  = (int2)  (get_global_id(0),   get_global_id(1));
    float2 offset = (float2)(get_global_size(0), get_global_size(1)) * 0.5;
    
    float d = length((float2)(coord.x,coord.y} - offset);
    float r = sin(t * -0.7 + d * 0.13) * 0.5 + 0.5;
    float g = sin(t * -1.3 + d * 0.11) * 0.5 + 0.5;
    float b = sin(t * -1.9 + d * 0.10) * 0.5 + 0.5;
    
    write_imagef(framebuffer, coord, (float4)(r,g,b,1));
}
