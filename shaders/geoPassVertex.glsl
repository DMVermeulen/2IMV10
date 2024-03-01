#version 330 core

layout( location = 0 ) in vec3 pos;
layout( location = 1 ) in vec3 normal;
layout( location = 2 ) in vec3 direction;

out vec3 worldPos;
out vec3 N;
out vec3 dir;

uniform mat4 view;
uniform mat4 proj;

void main() {
	gl_Position = proj*view*vec4( pos, 1.0f );
	//gl_Position = vec4( pos, 1.0f );
	worldPos = pos;
	N = normal;
	dir = direction;
	
	//dir=vec3(0.5,0.5,0);
}