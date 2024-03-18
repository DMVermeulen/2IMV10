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
	void drawLineMode(float lineWidth);
	void recreateMeshNewRadius(float radius, int nTris);
	void recreateMeshNewNTris(float radius, int nTris);
	void edgeBundling(float p, float radius, int nTris);
	void edgeBundlingGPU(float p, float radius, int nTris);
	void edgeBundlingCUDA(float p, float radius, int nTris);
	void initSSBOBinding();
	int getNVoxelsX();
	int getNVoxelsY();
	int getNVoxelsZ();
	glm::vec3 getAABBMin();
	float getVoxelUnitSize();
	int getTotalVoxels();
	GLuint getDenseMap();
	GLuint getVoxelCount();
	void updateEnableSlicing();
	void slicing(glm::vec3 pos, glm::vec3 dir);

	//DEBUG
	void testSmoothing();

private:
	//std::vector<std::vector<glm::vec3>> tracks;
	std::vector<glm::vec3> tracks;     //stores the original tracks without bundling
	std::vector<uint32_t> trackOffset;
	std::vector<uint32_t> trackSize;
	std::vector<Tube>tubes;            //persistently store this for recreating meshes when parameter changes
	std::vector<int>isFiberEndpoint;        //store for each vertex whether it is an endpoint of some fiber
	std::vector<Vertex>vertices;
	std::vector<uint32_t>indices;
	std::vector<glm::vec3> normals;     //normal vectors for each vertex, as a vertex attribute
	std::vector<glm::vec3> directions;  //directions for each vertex, as a vertex attribute

	bool visible=true;
	GLuint VAO = -1;
	GLuint VBO = -1;
	GLuint IBO = -1;

	//For line mode drawing
	GLuint VAOLines = -1;   
	GLuint VBOLines = -1;   
	GLuint NBOLines = -1;   //lineNormal
	GLuint DBOLines = -1;   //lineDirection
	
	//std::vector<Vertex>verticesLineMode;

	void loadTracksFromTCK(std::string path);
	void updateTubes(std::vector<glm::vec3>& currentTracks);
	void initLineNormals();
	void initLineDirections();
	void updateTriangles(float radius, int nTris);
	void updateVertexIndiceBuffer();
	std::vector<glm::vec3> readTCK(const std::string& filename);
	void initVertexBufferLineMode();

	void trackResampling();

	//edge-bundling related structures on host 
	uint32_t nVoxels_Z = 400;  //500
	uint32_t nVoxels_X;
	uint32_t nVoxels_Y;
	float voxelUnitSize;
	int totalVoxels;
	AABB aabb;   //bounding box for the instance
	int nIters = 15;  //number of iterations for edge bundling
	const float smoothFactor = 0.99; 
	float smoothL;
	const float relaxFactor = 0.85;
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
	ComputeShader denseEstimationShaderX;
	ComputeShader denseEstimationShaderY;
	ComputeShader denseEstimationShaderZ;
	ComputeShader advectionShader;
	ComputeShader smoothShader;
	ComputeShader relaxShader;
	ComputeShader updateDirectionShader;
	ComputeShader updateNormalShader;
	ComputeShader forceConsecutiveShader;

	//GPU passes for edge bundling
	void voxelCountPass();
	void denseEstimationPass(float p);
	void advectionPass(float p);
	void smoothPass(float smoothFactor);
	void relaxationPass();
	void updateDirectionPass();
	void updateNormalPass();
	void forceConsecutivePass();
	
	//helper functions
	void transferDataGPU(GLuint srcBuffer, GLuint dstBuffer, size_t copySize);

	//textures used on GPU
	//GLuint texOriTracks;
	//GLuint texTempTracks;     //store intermediate result for each iteration, initialized to oriTracks
	GLuint texVoxelCount;  //Todo: now it's generated using CPU, can be performed by GPU using atomic add
	GLuint texDenseX;
	GLuint texDenseY;
	GLuint texDenseMap;
	//GLuint texUpdatedTracks;  
	GLuint debugUINT;
	GLuint debugFLOAT;
	GLuint texTempTubes;
	GLuint texUpdatedTubes;
	GLuint texSmoothedTubes;
	GLuint texRelaxedTubes;
	GLuint texIsFiberEndpoint;
	GLuint texTempNormals;
	GLuint texUpdatedNormals;
	GLuint texTempDirections;
	GLuint texUpdatedDirections;
	void initTextures();

	//Slicing related
	bool enableSlicling=false;
	glm::vec3 slicingPos;
	glm::vec3 slicingDir;
	ComputeShader slicingShader;


	//TEST
	// CUDA related
	//pointers to GPU memory
	int* d_voxelCount;
	float* d_denseMap;
	float* d_tempTubes;
	float* d_updatedTubes;
	float* d_smoothedTubes;
	float* d_relaxedTubes;
	int* d_isFiberEndpoint;
	float* d_tempNormals;
	float* d_updatedNormals;
	float* d_tempDirections;
	float* d_updatedDirections;
	void initCudaMemory();
	void transferDataHostToGL(void* hostMem, GLuint glBuffer, size_t copySize);

};