#version 420 compatibility

in vec3 vertex;
out vec3 pos;

void main (void)
{
	pos = vertex;
}