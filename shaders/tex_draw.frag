
#version 330

in vec2 tex_coord;

uniform sampler2D framebuffer;


void main (void)
{
    gl_FragColor = texture(framebuffer, tex_coord);
}
