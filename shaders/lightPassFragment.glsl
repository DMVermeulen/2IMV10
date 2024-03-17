#version 430 core

#define COLOR_MODE_DIRECTION 0
#define COLOR_MODE_DENSITY 1
#define COLOR_MODE_GRADIENT 2

in vec2 UV;
out vec4 fragColor;

uniform sampler2D worldPos;
uniform sampler2D gDir;
uniform sampler2D gNormal;
uniform vec3 viewPos;
uniform int colorMode;
uniform sampler3D denseMap;

uniform int nVoxels_X;
uniform int nVoxels_Y;
uniform int nVoxels_Z;
uniform vec3 aabbMin;
uniform float voxelUnitSize;
uniform int totalVoxels;

const vec3 lightPos=vec3(0,0,100);

float sigmoid(float x) {
    return 1.0f / (1.0f + exp(-x));
}

void main() {
	vec3 fragPos = texture(worldPos, UV).rgb;
	vec3 normal = texture(gNormal, UV).rgb;
	vec3 dir = texture(gDir,UV).xyz;
	vec3 albedo = normalize(abs(dir));
	//vec3 albedo = vec3(1,0,0);
	
	//Phong shading
	vec3 V = normalize(viewPos-fragPos);
	vec3 L = normalize(lightPos-fragPos);
	vec3 H = normalize(L+V);

	float diffuse = max( dot( L, normal ), 0.0f );

	float specular = pow( max( dot( H, normal ), 0.0f ), 64.0f );
	//specular = max(specular,0);
    if(diffuse<=0)
	 specular = 0;

	//fragColor = vec4(1*(diffuse + specular)*albedo,1.0);
	
	//NO LIGHTING
    if (colorMode == COLOR_MODE_DIRECTION) {
	   fragColor = vec4(albedo,1.0);
    } 
	else if (colorMode == COLOR_MODE_DENSITY) {
	   vec3 deltaP = fragPos - aabbMin;
	   int level_X = int(min(float(nVoxels_X - 1), deltaP.x / voxelUnitSize));
	   int level_Y = int(min(float(nVoxels_Y - 1), deltaP.y / voxelUnitSize));
	   int level_Z = int(min(float(nVoxels_Z - 1), deltaP.z / voxelUnitSize));
	   
	   vec3 texCoord = vec3(1.0*level_X/nVoxels_X,1.0*level_Y/nVoxels_Y,1.0*level_Z/nVoxels_Z);
	   float dense = texture(denseMap,texCoord).r;
	   fragColor = vec4(sigmoid(dense),0,0,1.0);
    } 
	else if (colorMode == COLOR_MODE_GRADIENT) {
	
    } 
	else {
	
    }
}