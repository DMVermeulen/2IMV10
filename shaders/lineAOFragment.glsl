#version 330 core

out vec4 FragColor;

in vec2 UV;

uniform sampler2D shadedColor;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gDir;
uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform float radius;
uniform float colorInterval;

// parameters (you'd probably want to use them as uniforms to more easily tweak the effect)
int kernelSize = 64;
//float radius = 10;
float bias = 0.025;

// tile noise texture over screen based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(1280.0/4.0, 720.0/4.0); 

uniform mat4 view;
uniform mat4 proj;
uniform vec3 viewPos;

//const vec3 ambient = vec3(0.1,0.1,0.1);
const vec3 ambient = vec3(0.1,0,0);

//LineAO parameter settings
int nScale = 3;
int nSampleBase = 64;
float radiusBase = ;
float deltaR = ;


void main()
{
    // get input for SSAO algorithm
    vec3 fragPos = texture(gPosition, UV).xyz;
	vec3 dir = texture(gDir, UV).xyz;
	vec3 normal = normalize(cross(cross(dir,viewPos-fragPos),dir));
    vec3 randomVec = normalize(texture(texNoise, UV * noiseScale).xyz);

    vec3 tangent = normalize(texture(gDir, UV).xyz);
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
	
    float occlusionTotal = 0.0;
	int nSample = nSampleBase;
	int sampleStep = 1;
	
	// iterate over multiple scales
    for(int k=0;k<nScale;k++){
	   float radius = radiusBase+k*deltaR;
	   float occlusionStep = 0;
	   // iterate over sample points for that scale
	   for(int i=0;i<kernelSize;i+=sampleStep){
	       // get sample position
           vec3 samplePos = TBN * samples[i]; // from tangent to world-space
           samplePos = fragPos + samplePos * radius; 
        
           // project sample position (to get position on screen/texture)
           vec4 offset = vec4(samplePos, 1.0);
           offset = proj * view * offset;        // from world to clip-space
           offset.xyz /= offset.w;               // perspective divide
           offset.xyz = offset.xyz * 0.5 + 0.5;  // transform to range 0.0 - 1.0
  
           // get sample depth
           float sampleDepth = texture(gPosition, offset.xy).z; // get depth value of kernel sample
        
		   vec4 pivotPos=vec4(texture(gPosition, offset.xy).xyz,1);
		   pivotPos=view*pivotPos;
		   float pivotDepth=pivotPos.z;
		
		   float myDepth=(view*vec4(fragPos,1)).z;
		
           // check for occlusion
		   float rangeCheck = smoothstep(0.0, 1.0, radius / abs(myDepth - pivotDepth));
           occlusionStep += (pivotDepth >= myDepth + bias ? 1.0 : 0.0);//* rangeCheck; 	
	   }
	   occlusionStep = 1.0-(occlusionStep/nSample);
	   occlusionTotal += occlusionStep;
	   nSample/=2;
	   sampleStep*=2;
	}
	
	FragColor = vec4(occlusionTotal * (texture(shadedColor,UV).xyz+ambient), 1.0f);
	
	//color flattening
	if(colorInterval>1e-4 && FragColor.x>0){
	   	int coord_X = int(FragColor.x/colorInterval);
	    int coord_Y = int(FragColor.y/colorInterval);
	    int coord_Z = int(FragColor.z/colorInterval);
	    vec3 coord = vec3(coord_X,coord_Y,coord_Z);
		vec3 extra = 0.5*vec3(colorInterval,colorInterval,colorInterval);
	    FragColor = vec4(colorInterval*coord+extra,1.0f);
	}
		
	if(dir.x==0)
	  FragColor = vec4(0,0,0,1);
}