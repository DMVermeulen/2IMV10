#version 330 core

layout( location = 0 ) in vec3 pos;

out vec3 worldPos;

uniform mat4 view;
uniform mat4 proj;

void main() {
	gl_Position = proj*view*vec4( pos, 1.0f );
	//gl_Position = vec4( pos, 1.0f );
	worldPos = pos;
}