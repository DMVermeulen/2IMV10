#version 330 core

in vec3 worldPos;
in vec3 N;
in vec3 dir;
in vec2 motionVectorScreen;

layout(location = 0) out vec4 outPos;
layout(location = 1) out vec4 outN;
layout(location = 2) out vec4 outDir;
layout(location = 3) out vec4 outMotionVectorScreen;

void main() {

    outPos = vec4(worldPos,0);
	outN = vec4(N,0);
	outDir = vec4(dir,0);
	outMotionVectorScreen = vec4(motionVectorScreen,0,0);
	
	//dir=vec4(0.5,0.5,0,0);
}