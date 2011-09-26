
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

    vec4 c = vec4(texelFetch(framebuffer, pos).xyz, 1);

    // Do gamma correction
    c = vec4(pow(c.r, 0.454545), pow(c.g, 0.4545), pow(c.b, 0.4545), c.a);

    gl_FragColor = c;
}
