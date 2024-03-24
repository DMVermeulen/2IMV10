#version 330 core

out vec4 FragColor;

in vec2 UV;

uniform sampler2D imageData;

uniform float sharpening;

void main()
{
    vec3 color = texture(imageData, UV).rgb;
	
	// Apply sharpening
	const mat3 kernel = mat3(
        -1.0, -1.0, -1.0,
        -1.0,  9.0, -1.0,
        -1.0, -1.0, -1.0
    );

    vec3 topLeft = texture(imageData, UV + vec2(-1.0, -1.0)).rgb; // Change TexCoord to UV
    vec3 top = texture(imageData, UV + vec2(0.0, -1.0)).rgb; // Change TexCoord to UV
    vec3 topRight = texture(imageData, UV + vec2(1.0, -1.0)).rgb; // Change TexCoord to UV
    vec3 left = texture(imageData, UV + vec2(-1.0, 0.0)).rgb; // Change TexCoord to UV
    vec3 right = texture(imageData, UV + vec2(1.0, 0.0)).rgb; // Change TexCoord to UV
    vec3 bottomLeft = texture(imageData, UV + vec2(-1.0, 1.0)).rgb; // Change TexCoord to UV
    vec3 bottom = texture(imageData, UV + vec2(0.0, 1.0)).rgb; // Change TexCoord to UV
    vec3 bottomRight = texture(imageData, UV + vec2(1.0, 1.0)).rgb; // Change TexCoord to UV

    vec3 sharpenedColor = color + sharpening * (
        topLeft * kernel[0][0] + top * kernel[0][1] + topRight * kernel[0][2] +
        left * kernel[1][0] + color * kernel[1][1] + right * kernel[1][2] +
        bottomLeft * kernel[2][0] + bottom * kernel[2][1] + bottomRight * kernel[2][2]
    );
    FragColor = vec4(sharpenedColor, 1.0);
	
    if(color.x==0)
	  FragColor = vec4(0,0,0,1);
	
}