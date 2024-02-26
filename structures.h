#pragma once
#include"glm/glm.hpp"

struct Tube {
	glm::vec3 p1;
	glm::vec3 p2;
};

struct Vertex {
	glm::vec3 pos;
	//add other attributes(e.g., color, normal...)
};

struct AABB {
	glm::vec3 minPos;
	glm::vec3 maxPos;
};