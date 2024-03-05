#version 330 core

out vec4 FragColor;

in vec2 UV;

uniform sampler2D sampleTexture;

void main()
{
    vec3 color = texture(sampleTexture, UV).xyz;

    FragColor = vec4(normalize(color), 1.0f);
	
}