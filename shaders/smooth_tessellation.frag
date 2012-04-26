#version 420 compatibility

in vec4 color;
out vec4 frag_color;

void main (void)
{
    frag_color = pow(color, vec4(1.0/2.2));
}
