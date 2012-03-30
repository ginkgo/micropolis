
#version 150

uniform mat4 projection;

in vec3 vertex;


void main()
{
	gl_Position  = projection * vec4(vertex,1);
}
