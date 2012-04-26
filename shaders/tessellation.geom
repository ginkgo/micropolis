#version 420 compatibility

layout(triangles) in;
<<<<<<< HEAD
		  layout(triangle_strip, max_vertices = 3) out;

		  in vec3 pos[3];
		  out vec4 color;

		  void main()
{

    vec3 u = pos[2] - pos[0];
    vec3 w = pos[1] - pos[0];

    vec3 n = -normalize(cross(u,w));
=======
layout(triangle_strip, max_vertices = 3) out;

in vec3 pos[3];
out vec4 color;

void main()
{
	//color = vec4(1,1,1,1);
	//gl_Position = gl_in[0].gl_Position;
//
	//EmitVertex();
	//EndPrimitive();

	vec3 u = pos[2] - pos[0];
	vec3 w = pos[1] - pos[0];

	vec3 n = -normalize(cross(u,w));
>>>>>>> 769959448920ee9a51d59396cba17ca2c9a2fbfb

    vec3 l = normalize(vec3(4,3,8));

    vec3 v = -normalize(pos[0]+pos[1]+pos[2]);

    vec4 dc = vec4(0.8, 0.05, 0.01, 1);
    vec4 sc = vec4(1, 1, 1, 1);
        
    vec3 h = normalize(l+v);
        
    float sh = 30.0;

    vec4 c = max(dot(n,l),0.0) * dc + pow(max(dot(n,h), 0.0), sh) * sc;
	
<<<<<<< HEAD
    color = c;
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    color = c;
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    color = c;
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();


    EndPrimitive();
}
=======
	color = c;
	gl_Position = gl_in[0].gl_Position;
	EmitVertex();

	color = c;
	gl_Position = gl_in[1].gl_Position;
	EmitVertex();

	color = c;
	gl_Position = gl_in[2].gl_Position;
	EmitVertex();


	EndPrimitive();
}
>>>>>>> 769959448920ee9a51d59396cba17ca2c9a2fbfb
