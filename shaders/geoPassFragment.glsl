#version 330 core

in vec3 worldPos;
in vec3 N;
in vec3 dir;

layout(location = 0) out vec4 outPos;
layout(location = 1) out vec4 outN;
layout(location = 2) out vec4 outDir;

void main() {

    outPos = vec4(worldPos,0);
	outN = vec4(N,0);
	outDir = vec4(dir,0);
	
	//dir=vec4(0.5,0.5,0,0);
}