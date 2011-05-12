
#version 330

in  vec2 vertex;
out vec2 tex_coord;

void main (void)
{
    gl_Position = vec4(vertex,0,1);
    tex_coord = vertex * 0.5 + vec2(0.5);
}
