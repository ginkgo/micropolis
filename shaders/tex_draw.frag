
#version 330

uniform samplerBuffer framebuffer;
uniform int bsize;
uniform ivec2 gridsize;

void main (void)
{
    ivec2 pxlpos = ivec2(gl_FragCoord.xy);

    ivec2 gridpos = pxlpos / bsize;
    int   grid_id = gridpos.x + gridsize.x * gridpos.y;
    ivec2 loclpos = pxlpos - gridpos * bsize;
    int   locl_id = loclpos.x + bsize * loclpos.y;

    int pos = grid_id * bsize * bsize + locl_id;

    gl_FragColor = texelFetch(framebuffer, pos);
}
