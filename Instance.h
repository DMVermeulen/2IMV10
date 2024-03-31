#pragma once

#include<vector>
#include<string>
#include"structures.h"
#include"glad/glad.h"
#include"ComputeShader.h"
#include"Enums.h"

class Instance {
public:
	Instance(
		std::string path, 
		std::shared_ptr<ComputeShader> voxelCountShader,
		std::shared_ptr<ComputeShader> denseEstimationShaderX,
		std::shared_ptr<ComputeShader> denseEstimationShaderY,
		std::shared_ptr<ComputeShader> denseEstimationShaderZ,
		std::shared_ptr<ComputeShader> advectionShader,
		std::shared_ptr<ComputeShader> smoothShader,
		std::shared_ptr<ComputeShader> relaxShader,
		std::shared_ptr<ComputeShader> updateDirectionShader,
		std::shared_ptr<ComputeShader> slicingShader,
		std::shared_ptr<ComputeShader> _trackToLinesShader,
		std::shared_ptr<ComputeShader> denseEstimationShader3D
	);
	~Instance();
	int getNumberVertices();
	int getNumberIndices();
	int getNumberTriangles();
	std::vector<Vertex>& getVertices();
	std::vector<uint32_t>& getIndices();
	void setVisible(bool visible);
	void drawLineMode(float lineWidth);
	//void recreateMeshNewRadius(float radius, int nTris);
	//void recreateMeshNewNTris(float radius, int nTris);
	//void edgeBundling(float p, float radius, int nTris);
	void edgeBundlingGPU(float p);
	//void edgeBundlingCUDA(float p, float radius, int nTris);
	int getNVoxelsX();
	int getNVoxelsY();
	int getNVoxelsZ();
	glm::vec3 getAABBMin();
	float getVoxelUnitSize();
	int getTotalVoxels();
	GLuint getDenseMap();
	GLuint getVoxelCount();
	void updateEnableSlicing(glm::vec3 pos, glm::vec3 dir);
	//void slicingReset();
	void slicing(glm::vec3 pos, glm::vec3 dir);
	void setMaterial(float roughness, float metallic);
	void getMaterial(float* roughness, float* metallic);
	void getSettings(float* bundle, bool* _enableSlicing, glm::vec3* slicePos, glm::vec3* sliceDir);
	void activate();
	void deactivate();

private:
	bool isActivated = false;
	//std::vector<std::vector<glm::vec3>> tracks;
	std::vector<glm::vec3> tracks;     //stores the original tracks without bundling
	std::vector<uint32_t> trackOffset;
	std::vector<uint32_t> trackSize;
	//std::vector<Tube>tubes;            //persistently store this for recreating meshes when parameter changes
	//std::vector<int>isFiberEndpoint;      
	std::vector<int>trackId;             //store for each point which track it belongs to
	std::vector<int>isFiberBegin;        //store for each vertex whether it is a begin point of some fiber
	std::vector<int>isFiberEnd;
	std::vector<Vertex>vertices;
	std::vector<uint32_t>indices;
	//std::vector<glm::vec3> normals;     //normal vectors for each vertex, as a vertex attribute
	//std::vector<glm::vec3> directions;  //directions for each vertex, as a vertex attribute

	bool visible=true;
	GLuint VAO = -1;
	GLuint VBO = -1;
	GLuint IBO = -1;

	//For line mode drawing
	GLuint VAOLines = -1;   
	GLuint VBOLines = -1;   
	GLuint DBOLines = -1;   //lineDirection
	int lineBufferSize=0;
	
	//std::vector<Vertex>verticesLineMode;

	void loadTracksFromTCK(std::string path);
	//void updateTriangles(float radius, int nTris);
	//void updateVertexIndiceBuffer();
	std::vector<glm::vec3> readTCK(const std::string& filename);
	void trackResampling();

	//edge-bundling related structures on host 
	float bundle=0;
	uint32_t nVoxels_Z = 420;  //500
	uint32_t nVoxels_X;
	uint32_t nVoxels_Y;
	float voxelUnitSize;
	int totalVoxels;
	AABB aabb;   //bounding box for the instance
	int nIters = 15;  //number of iterations for edge bundling
	//const float smoothFactor = 0.99; 
	//float smoothL;
	//const float relaxFactor = 0.85;
	float smoothFactor = 0.99;
	float smoothL;
	float relaxFactor = 0.85;
	std::vector<uint32_t> voxelAssignment; //persistently stored
	std::vector<uint32_t> voxelOffset; 
	std::vector<uint32_t> voxelSize; 
	std::vector<uint32_t> voxelCount;
	void spaceVoxelization();  //called only once when the instance is initialized
	void resampling();

	// CPU-based edge bundling
	//std::vector<float> computeDensity(float p);    //called every time when user-specified parameter changes (the kernel width)
	//std::vector<glm::vec3> advection(std::vector<float>& denseMap,float p);   //called every time when user-specified parameter changes (the kernel width)
	//std::vector<glm::vec3> smoothing(std::vector<glm::vec3>& newTracks);         //called every time when user-specified parameter changes (the kernel width)

	//GPU passes for edge bundling
	void voxelCountPass();
	void denseEstimationPass(float p);
	void advectionPass(float p);
	void smoothPass(float smoothFactor);
	void relaxationPass();
	void updateDirectionPass();
	void trackToLinePass(GLuint texTrack, GLuint texLines);

	
	//helper functions
	void transferDataGPU(GLuint srcBuffer, GLuint dstBuffer, size_t copySize);

	//textures used on GPU
	//GLuint texOriTracks;
	GLuint texVoxelCount; 
	GLuint texDenseX;
	GLuint texDenseY;
	GLuint texDenseMap;
	//GLuint texUpdatedTracks;  
	GLuint debugUINT;
	GLuint debugFLOAT;
	//GLuint texTempTubes;
	//GLuint texUpdatedTubes;
	//GLuint texSmoothedTubes;
	//GLuint texRelaxedTubes;
	GLuint texTempTracks;
	GLuint texUpdatedTracks;
	GLuint texSmoothedTracks;
	GLuint texRelaxedTracks;
	GLuint texIsFiberBegin;
	GLuint texIsFiberEnd;
	GLuint texTrackId;
	GLuint texRelaxedLines;

	//GLuint texIsFiberEndpoint;
	GLuint texTempNormals;
	GLuint texUpdatedNormals;
	GLuint texTempDirections;
	GLuint texUpdatedDirections;
	//void initTextures();
	void destroyTextures();
	void createTextures();
	void initAddition();

	//Slicing related
	bool enableSlicling=false;
	glm::vec3 slicingPos = glm::vec3(0);
	glm::vec3 slicingDir = glm::vec3(0);

	//PBR materials
	float roughness=0.2f;
	float metallic=0.8f;

	// Compute shaders
	std::shared_ptr<ComputeShader> voxelCountShader;
	std::shared_ptr<ComputeShader> denseEstimationShaderX;
	std::shared_ptr<ComputeShader> denseEstimationShaderY;
	std::shared_ptr<ComputeShader> denseEstimationShaderZ;
	std::shared_ptr<ComputeShader> advectionShader;
	std::shared_ptr<ComputeShader> smoothShader;
	std::shared_ptr<ComputeShader> relaxShader;
	std::shared_ptr<ComputeShader> updateDirectionShader;
	std::shared_ptr<ComputeShader> slicingShader;
	std::shared_ptr<ComputeShader> trackToLinesShader;

	////TEST
	//// CUDA related
	////pointers to GPU memory
	//int* d_voxelCount;
	//float* d_denseMap;
	//float* d_tempTubes;
	//float* d_updatedTubes;
	//float* d_smoothedTubes;
	//float* d_relaxedTubes;
	//int* d_isFiberEndpoint;
	//float* d_tempNormals;
	//float* d_updatedNormals;
	//float* d_tempDirections;
	//float* d_updatedDirections;
	//void initCudaMemory();
	//void transferDataHostToGL(void* hostMem, GLuint glBuffer, size_t copySize);

	//DEBUG
	std::vector<Tube>tubes;
	std::vector<glm::vec3>directions;
	std::vector<int>isFiberEndpoint;
	void updateTubes(std::vector<glm::vec3>& currentTracks);
	std::shared_ptr<ComputeShader> denseEstimationShader3D;
	GLuint texDenseMapTest;
};