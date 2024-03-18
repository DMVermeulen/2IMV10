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

const float roughness = 0.2;
const float metallic = 0.8;
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
	vec3 albedo = normalize(abs(dir));
	//vec3 albedo = normalize(abs(normal));
	//vec3 albedo = vec3(0,0,1);
	
	//Phong shading
	vec3 V = normalize(viewPos-fragPos);
	//vec3 L = normalize(lightPos-fragPos);
	//vec3 L = normalize(lightDir);
	vec3 L = normalize(vec3(0,0,1));
	vec3 H = normalize(L+V);

	float diffuse = max( dot( L, normal ), 0.0f );

	float specular = pow( max( dot( H, normal ), 0.0f ), 16.0f );
	//specular = max(specular,0);
    if(diffuse<=0)
	 specular = 0;


    if (colorMode == COLOR_MODE_DIRECTION) {
	   //Lighting
	   //fragColor = vec4(1*(diffuse + specular)*albedo,1.0);
	   
	   //NO LIGHTING
	   //fragColor = vec4(albedo,1.0);
	   //fragColor = vec4(abs(normalize(normal)),1.0);
	   
	   //PBR LIGHTING
	   vec3 F0 = vec3(0.04); 
       F0 = mix(F0, albedo, metallic);
	   vec3 Lo = vec3(0.0);
	   for(int i=0;i<34;i++){
	   L=lightDir[i];
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
		fragColor = vec4(color+ambient,1.0f);
		//fragColor = vec4(abs(normal),1);
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