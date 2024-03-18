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
	void drawAllInstancesLineMode(float lineWidth);
	void drawActivatedInstanceLineMode(float lineWidth);
	void setRadius(float r);
	void setNTris(int n);
	void edgeBundling(float p, float radius, int nTris);
	void slicing(glm::vec3 pos, glm::vec3 dir);
	void setActivatedInstance(int id);
	int getInstanceNVoxelsX();
	int getInstanceNVoxelsY();
	int getInstanceNVoxelsZ();
	glm::vec3 getInstanceAABBMin();
	float getInstanceVoxelUnitSize();
	int getInstanceTotalVoxels();
	GLuint getInstanceDenseMap();
	GLuint getInstanceVoxelCount();
	void updateInstanceEnableSlicing();
private:
	//parameters used to build triangles from streamlines
	float radius = 0.1f;
	int nTris = 3;
	
	int activatedInstance=0;

	std::vector<Instance> instances;
	void updateMeshNewRadius();
	void updateMeshNewNTris();

};