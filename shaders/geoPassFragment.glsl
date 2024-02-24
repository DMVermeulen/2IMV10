#version 330 core

in vec3 worldPos;

out vec4 outPos;

void main() {

    outPos = vec4(worldPos,0);
}