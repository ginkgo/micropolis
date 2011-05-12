
__kernel void test (write_only image2d_t framebuffer, float time)
{
    int2 coord = (int2){get_global_id(0), get_global_id(1)};
    
    float f = sin(time * 3 + length((float2){coord.x,coord.y}) * 0.1) * 0.5 + 0.5;

    write_imagef(framebuffer, coord, (float4){f,f,0,1});
}
