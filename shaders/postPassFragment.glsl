#version 330 core

out vec4 FragColor;

in vec2 UV;

uniform sampler2D imageWithAO;
uniform float contrast;
uniform float brightness;

void main()
{
    vec3 color = texture(imageWithAO, UV).xyz;
	
	// Apply contrast adjustment
    FragColor = vec4((color - 0.5) * contrast + 0.5, 1.0);
	
	//Apply brightness
	FragColor = vec4((FragColor.rgb + vec3(brightness)), 1.0);
	
	if(color.x==0)
	  FragColor = vec4(0,0,0,1);

}