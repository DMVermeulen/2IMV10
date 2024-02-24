#pragma once
#include<vector>
#include<string>
#include"structures.h"
#include"Instance.h"

class Scene {
public:
	Scene();
	~Scene();
	void addInstance(std::string path);
	void removeInstance(int insId);
	std::vector<Vertex>& getInstanceVertices(int instId);
	std::vector<uint32_t>& getInstanceIndicies(int instId);
	void drawAllInstances();
private:
	//parameters used to build triangles from streamlines
	float radius = 0.3f;
	float nTris = 3;

	std::vector<Instance> instances;
};