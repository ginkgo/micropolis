#version 420 compatibility

uniform mat4 proj;

in vec3 vertex;
out vec3 pos;
out vec4 hPos;

void main (void)
{
	pos = vertex;
	hPos = proj * vec4(vertex,1);
}