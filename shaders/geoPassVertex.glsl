#version 330 core

layout( location = 0 ) in vec3 pos;
layout( location = 1 ) in vec3 direction;

out vec3 worldPos;
out vec3 N;
out vec3 dir;
out vec2 motionVectorScreen;

uniform mat4 view;
uniform mat4 lastView;
uniform mat4 proj;
uniform vec3 viewPos;

void main() {
	gl_Position = proj*view*vec4( pos, 1.0f );
	worldPos = pos;
	dir = direction;
	
	vec3 viewDir = normalize(viewPos-pos);
	vec3 t = normalize(direction);
	vec3 offset = normalize( cross( viewDir, t ) );
	N = normalize( cross( offset, t ) );
	N *= sign( dot( N, viewDir ) );
	
	vec4 motion = proj*view*vec4( pos, 1.0f ) - proj*lastView*vec4( pos, 1.0f );
	motionVectorScreen = (motion/motion.w).xy;
	motionVectorScreen = normalize(motionVectorScreen);
}