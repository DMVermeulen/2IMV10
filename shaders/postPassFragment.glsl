#version 330 core

out vec4 FragColor;

in vec2 UV;

uniform sampler2D imageWithAO;
uniform sampler2D motionVectorScreen;
uniform float contrast;
uniform float brightness;
uniform float saturation;

//used for saturation 
const vec3 constantWeight = vec3(0.2125, 0.7154, 0.0721);  

//used for motion bluring
float blurL = 0.5;
float delta = 0.01;
float motionBlur = 0;


void main()
{
    vec3 color = texture(imageWithAO, UV).xyz;
	
	//Apply motion bluring
	vec3 blurColor = vec3(0);
	vec2 motionVector = texture(motionVectorScreen, UV).xy;
	if(abs(motionVector.x)>1e-5){
		int n = int(blurL/delta);
	    float weight = 0.6;
	    for(int k=0;k<n;k++){
	       vec2 XY = UV + motionVector*k*delta;
	       vec3 c = texture(imageWithAO, XY).rgb;
	       blurColor += c*weight;
	       weight/=2;
	    }
	    color = (1-motionBlur)*color + motionBlur*blurColor;
	}

	// Apply contrast adjustment
    FragColor = vec4((color - 0.5) * contrast + 0.5, 1.0);
	
	//Apply saturation
	float luminance = dot(FragColor.rgb, constantWeight);
    vec3 greyColor = vec3(luminance);
    FragColor = vec4(mix(greyColor, FragColor.rgb, saturation), 1.0);
	
	//Apply brightness
	FragColor = vec4((FragColor.rgb + vec3(brightness)), 1.0);
		
	if(color.x==0)
	  FragColor = vec4(0,0,0,1);

}