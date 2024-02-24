#version 330 core

in vec2 UV;
out vec4 fragColor;

uniform sampler2D worldPos;

void main() {
    vec3 color = texture(worldPos,UV).rgb;
	if(color.x==0)
	  fragColor = vec4(0,0,0,1.0f);
	else
	  fragColor= vec4(1,0,0,1);
	//fragColor = vec4(normalize(color)*5,1.0);
}