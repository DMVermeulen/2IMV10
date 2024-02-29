#pragma once

#include<vector>
#include<string>
#include"structures.h"
#include"glad/glad.h"
#include"ComputeShader.h"
#include"Enums.h"

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
	void draw(RenderMode mode);
	void recreateMeshNewRadius(float radius, int nTris);
	void recreateMeshNewNTris(float radius, int nTris);
	void edgeBundling(float p, float radius, int nTris);
	void edgeBundlingGPU(float p, float radius, int nTris);
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

	//For line mode drawing
	GLuint VAOLines = -1;   
	GLuint VBOLines = -1;   
	
	//std::vector<Vertex>verticesLineMode;

	void loadTracksFromTCK(std::string path);
	void updateTubes(std::vector<glm::vec3>& currentTracks);
	void updateTriangles(float radius, int nTris);
	void updateVertexIndiceBuffer();
	std::vector<glm::vec3> readTCK(const std::string& filename, int offset);
	void initVertexBufferLineMode();

	//edge-bundling related structures on host 
	const uint32_t nVoxels_Z = 100;
	uint32_t nVoxels_X;
	uint32_t nVoxels_Y;
	float voxelUnitSize;
	int totalVoxels;
	AABB aabb;   //bounding box for the instance
	const int nIters = 5;  //number of iterations for edge bundling
	std::vector<uint32_t> voxelAssignment; //persistently stored
	std::vector<uint32_t> voxelOffset; 
	std::vector<uint32_t> voxelSize; 
	std::vector<uint32_t> voxelCount;
	void spaceVoxelization();  //called only once when the instance is initialized
	void resampling();
	std::vector<float> computeDensity(float p);    //called every time when user-specified parameter changes (the kernel width)
	std::vector<glm::vec3> advection(std::vector<float>& denseMap,float p);   //called every time when user-specified parameter changes (the kernel width)
	std::vector<glm::vec3> smoothing(std::vector<glm::vec3>& newTracks);         //called every time when user-specified parameter changes (the kernel width)

	//Compute shaders to accelerate edge bundling 
	ComputeShader voxelCountShader;
	//ComputeShader voxelCountEndpointsShader;
	ComputeShader denseEstimationShader;
	ComputeShader advectionShader;

	//GPU passes for edge bundling
	void voxelCountPass();
	void denseEstimationPass(float p);
	//std::vector<glm::vec3> advectionPass(float p);
	void advectionPass(float p);
	
	//helper functions
	void transferDataGPU(GLuint srcBuffer, GLuint dstBuffer, size_t copySize);

	//textures used on GPU
	//GLuint texOriTracks;
	//GLuint texTempTracks;     //store intermediate result for each iteration, initialized to oriTracks
	GLuint texVoxelCount;  //Todo: now it's generated using CPU, can be performed by GPU using atomic add
	GLuint texDenseMap;
	//GLuint texUpdatedTracks;  
	GLuint debugUINT;
	GLuint debugFLOAT;
	GLuint texTempTubes;
	GLuint texUpdatedTubes;

	void initTextures();


};