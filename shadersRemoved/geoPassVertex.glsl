#version 330 core

layout( location = 0 ) in vec3 pos;
layout( location = 1 ) in vec3 normal;
layout( location = 2 ) in vec3 direction;

out vec3 worldPos;
out vec3 N;
out vec3 dir;

uniform mat4 view;
uniform mat4 proj;
uniform vec3 viewPos;

void main() {
	gl_Position = proj*view*vec4( pos, 1.0f );
	//gl_Position = vec4( pos, 1.0f );
	worldPos = pos;
	//N = normal;
	dir = direction;
	
	//dir=vec3(0.5,0.5,0);
	
	//TEST
	//vec3 up = vec3( 0.0f, 0.01f, 1.0f );
	//vec3 t = normalize(direction);
	//vec3 offset = normalize( cross( up, t ) );
	//N = normalize( cross( offset, t ) );
	//N *= sign( dot( N, vec3( 0.0f, 0.01f, 1.0f ) ) );
	
	vec3 viewDir = normalize(viewPos-pos);
	vec3 t = normalize(direction);
	vec3 offset = normalize( cross( viewDir, t ) );
	N = normalize( cross( offset, t ) );
	N *= sign( dot( N, viewDir ) );
	
	//DEBUG
    //N= vec3(0,1,0);
	//N=abs(N);
}