#version 330 core

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

const vec3 lightPos=vec3(0,0,100);


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
	
	fragColor = vec4(albedo,1.0);
}