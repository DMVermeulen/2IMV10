#pragma once
#include<vector>
#include"structures.h"
#include"glad/glad.h"
#include"ComputeShader.h"
#include"Instance.h"

class FiberBundler {
public:
	FiberBundler();
	~FiberBundler();

	void init();
	void edgeBundlingGPU(float p);
	void enable(Instance* instance);
	void disable();
	bool isEnabled();
	int getLineBufferSize();
	float getBundleValue();
	GLuint getBundledFibers();
	void updateInstanceFibers();
	void switcheToInstance(Instance* instance);
private:
	bool isEnable=false;
	//instance specific variables
	Instance* instance;   //pointer to the instance currently working on
	std::vector<int>trackId;             //store for each point which track it belongs to
	std::vector<int>isFiberBegin;        //store for each vertex whether it is a begin point of some fiber
	std::vector<int>isFiberEnd;
	int lineBufferSize = 0;
	AABB aabb;   //bounding box for the instance

	//persistently stored on HOST memory
	std::vector<glm::vec3> resampledTracks;   
	std::vector<glm::vec3> resampledDirections;
	//destroyed immediately when the textures are created
	std::vector<uint32_t> resampledTrackOffset;
	std::vector<uint32_t> resampledTrackSize;

	//universal parameter settings
	float resampleFactor = 120;
	uint32_t nVoxels_Z = 400;  //500
	uint32_t nVoxels_X;
	uint32_t nVoxels_Y;
	float voxelUnitSize;
	int totalVoxels;
	int nIters = 15;  //number of iterations for edge bundling
	float smoothFactor = 0.99;
	float smoothL;
	float relaxFactor = 0.85;

	//dynamically updated parameters
	float bundle = 0;

	//textures used on GPU
	GLuint texVoxelCount;
	GLuint texDenseX;
	GLuint texDenseY;
	GLuint texDenseMap;
	GLuint texTempTracks;
	GLuint texUpdatedTracks;
	GLuint texTempDirections;
	GLuint texUpdatedDirections;
	GLuint texUpdatedDirectionsLines;
	GLuint texSmoothedTracks;
	GLuint texRelaxedTracks;
	GLuint texIsFiberBegin;
	GLuint texIsFiberEnd;
	GLuint texTrackId;
	GLuint texRelaxedLines;

	//Compute shaders to perform fiber bundling
	ComputeShader* voxelCountShader;
	ComputeShader* denseEstimationShaderX;
	ComputeShader* denseEstimationShaderY;
	ComputeShader* denseEstimationShaderZ;
	ComputeShader* advectionShader;
	ComputeShader* smoothShader;
	ComputeShader* relaxShader;
	ComputeShader* updateDirectionShader;
	ComputeShader* slicingShader;
	ComputeShader* trackToLinesShader;
	ComputeShader* denseEstimationShader3D;

	void initComputeShaders();
	void trackResampling();
	void spaceVoxelization();
	void initTextures();
	void destroyTextures();
	void cleanHostMemory();

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
	void initForInstance();
};
