#version 430 core

#define COLOR_MODE_DIRECTION 0
#define COLOR_MODE_NORMAL 1
#define COLOR_MODE_CONSTANT 2

#define LIGHTING_MODE_NORMAL 0
#define LIGHTING_MODE_PBR 1

in vec2 UV;
out vec4 fragColor;

uniform sampler2D worldPos;
uniform sampler2D gDir;
uniform sampler2D gNormal;
uniform vec3 viewPos;
uniform sampler3D denseMap;

uniform int nVoxels_X;
uniform int nVoxels_Y;
uniform int nVoxels_Z;
uniform vec3 aabbMin;
uniform float voxelUnitSize;
uniform int totalVoxels;

uniform float roughness;
uniform float metallic;

uniform int colorMode;
uniform vec3 constantAlbedo;

uniform int lightingMode;

const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------

float sigmoid(float x) {
    return 1.0f / (1.0f + exp(-x));
}

const vec3 lightDir[34] = {
    vec3(1,0,0),
	vec3(-1,0,0),
	vec3(0,1,0),
	vec3(0,-1,0),
	vec3(0,0,1),
	vec3(0,0,-1),
	
	vec3(1,1,0),
	vec3(-1,1,0),
	vec3(1,-1,0),
	vec3(-1,-1,0),
	vec3(1,0,1),
	vec3(1,0,-1),
    vec3(-1,0,1),
	vec3(-1,0,-1),
	vec3(0,1,1),
	vec3(0,1,-1),
    vec3(0,-1,1),
	vec3(0,-1,-1),
	
	vec3(1,1,1),
	vec3(1,1,-1),
	vec3(1,-1,1),
	vec3(1,-1,-1),
	vec3(-1,1,1),
	vec3(-1,1,-1),
	vec3(-1,-1,1),
	vec3(-1,-1,-1),
	
	vec3(1,2,1),
	vec3(1,2,-1),
	vec3(1,-2,1),
	vec3(1,-2,-1),
	vec3(-1,2,1),
	vec3(-1,2,-1),
	vec3(-1,-2,1),
	vec3(-1,-2,-1),
	
	
};

const vec3 ambient = vec3(0,0,0.1);

void main() {
	vec3 fragPos = texture(worldPos, UV).rgb;
	vec3 normal = texture(gNormal, UV).rgb;
	vec3 dir = texture(gDir,UV).xyz;
	vec3 albedo;
    switch (colorMode) {
        case COLOR_MODE_DIRECTION:
            albedo = normalize(abs(dir));
            break;
        case COLOR_MODE_NORMAL:
            albedo = normalize(abs(normal));
            break;
        case COLOR_MODE_CONSTANT:
            albedo = constanAlbedo;
            break;
        default:
            albedo = vec3(0,0,1);
            break;
    }
		
	if(LIGHTING_MODE_NORMAL == lightingMode){
	   fragColor = vec4(albedo,1.0);
	}
	else if(LIGHTING_MODE_PBR == lightingMode){
	   vec3 V = normalize(viewPos-fragPos);
	   vec3 F0 = vec3(0.04); 
       F0 = mix(F0, albedo, metallic);
	   vec3 Lo = vec3(0.0);
	   for(int i=0;i<34;i++){
	   vec3 L=lightDir[i];
	   H = normalize(L+V);
       // calculate per-light radiance
       vec3 radiance = vec3(1,1,1);        
        
       // cook-torrance brdf
       float NDF = DistributionGGX(normal, H, roughness);        
       float G   = GeometrySmith(normal, V, L, roughness);      
       vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
       vec3 kS = F;
       vec3 kD = vec3(1.0) - kS;
       kD *= 1.0 - metallic;	  
        
       vec3 numerator    = NDF * G * F;
       float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0) + 0.0001;
       vec3 specular     = numerator / denominator;  
            
       // add to outgoing radiance Lo
       float NdotL = max(dot(normal, L), 0.0);                
       Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
	   }
	   vec3 color = Lo;
	   color = color / (color + vec3(1.0));
       color = pow(color, vec3(1.0/2.2));  
	   fragColor = vec4(1.3*color+ambient,1.0f);
	}
}