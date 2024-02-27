#pragma once

#include<vector>
#include<string>
#include"structures.h"
#include"glad/glad.h"
#include"ComputeShader.h"

class Instance {
public:
	Instance(std::string path, float radius, int nTris);
	~Instance();
	int getNumberVertices();
	int getNumberIndices();
	int getNumberTriangles();
	std::vector<Vertex>& getVertices();
	std::vector<uint32_t>& getIndices();
	void setVisible(bool visible);
	void draw();
	void recreateMeshNewRadius(float radius, int nTris);
	void recreateMeshNewNTris(float radius, int nTris);
	void edgeBundling(float p, float radius, int nTris);
private:
	//std::vector<std::vector<glm::vec3>> tracks;
	std::vector<glm::vec3> tracks;     //stores the original tracks without bundling
	std::vector<uint32_t> trackOffset;
	std::vector<uint32_t> trackSize;
	std::vector<Tube>tubes;            //persistently store this for recreating meshes when parameter changes
	std::vector<Vertex>vertices;
	std::vector<uint32_t>indices;
	bool visible=true;
	GLuint VAO = -1;
	GLuint VBO = -1;
	GLuint IBO = -1;

	void loadTracksFromTCK(std::string path);
	void updateTubes(std::vector<glm::vec3>& currentTracks);
	void updateTriangles(float radius, int nTris);
	void updateVertexIndiceBuffer();
	std::vector<glm::vec3> readTCK(const std::string& filename, int offset);

	//edge-bundling related
	const uint32_t nVoxels_Z = 50;
	uint32_t nVoxels_X;
	uint32_t nVoxels_Y;
	float voxelUnitSize;
	AABB aabb;   //bounding box for the instance
	std::vector<uint32_t> voxelAssignment; //persistently stored
	std::vector<uint32_t> voxelOffset; 
	std::vector<uint32_t> voxelSize; 
	std::vector<uint32_t> voxelCount;
	void spaceVoxelization();  //called only once when the instance is initialized
	void resampling();
	std::vector<float> computeDensity(float p);    //called every time when user-specified parameter changes (the kernel width)
	std::vector<glm::vec3> advection(std::vector<float>& denseMap,float p);         //called every time when user-specified parameter changes (the kernel width)
	std::vector<glm::vec3> smoothing(std::vector<glm::vec3>& newTracks);         //called every time when user-specified parameter changes (the kernel width)

	//Compute shaders to accelerate edge bundling 
	ComputeShader denseEstimationShader;
	ComputeShader advectionShader;

};