#pragma once
#include<vector>
#include<string>
#include"structures.h"
#include"Instance.h"
#include"FiberBundler.h"

class Scene {
public:
	Scene();
	~Scene();
	void initComputeShaders();
	void addInstance(std::string path);
	void removeInstance(int insId);
	std::vector<Vertex>& getInstanceVertices(int instId);
	std::vector<uint32_t>& getInstanceIndicies(int instId);
	void drawAllInstancesLineMode(float lineWidth);
	void drawActivatedInstanceLineMode(float lineWidth);
	//void setRadius(float r);
	//void setNTris(int n);
	void enableFiberBundling();
	void edgeBundling(float p);
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
	void updateInstanceEnableSlicing(glm::vec3 pos, glm::vec3 dir,bool enable);
	void setInstanceMaterial(float roughness, float metallic);
	void getInstanceMaterial(float* roughness, float* metallic);
	bool isEmpty();
	void getInstanceSettings(float* bundle, bool* enableSlicing, glm::vec3* slicePos, glm::vec3* sliceDir);
	void updateFiberBundlingStatus(bool enable);
	void setBundlerAccuracy(float acc);
	float getRequiredVideoMem(float acc);
private:
	//parameters used to build triangles from streamlines
	float radius = 0.1f;
	int nTris = 3;
	
	int activatedInstance=0;

	std::vector<Instance> instances;
	//void updateMeshNewRadius();
	//void updateMeshNewNTris();

	//Compute shaders to accelerate edge bundling, shared by all instances
	ComputeShader* slicingShader;
	ComputeShader* trackToLinesShader;

	FiberBundler bundler;
};