#version 330 core

out vec4 FragColor;

in vec2 UV;

uniform sampler2D imageWithAO;
uniform float contrast;

void main()
{
    vec3 color = texture(imageWithAO, UV).xyz;
	
	// Apply contrast adjustment
    FragColor = vec4((color - 0.5) * contrast + 0.5, 1.0);

}