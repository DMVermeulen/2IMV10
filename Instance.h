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
		std::shared_ptr<ComputeShader> slicingShader,
		std::shared_ptr<ComputeShader> _trackToLinesShader
	);
	~Instance();
	const std::vector<glm::vec3>& getTracks();
	const std::vector<uint32_t>& getTrackOffset();
	const std::vector<uint32_t>& getTrackSize();
	void setVisible(bool visible);
	void drawLineMode(float lineWidth, int pointCount);
	void setBundleValue(float bundle);
	float getBundleValue();
	glm::vec3 getAABBMin();
	void setEnableSlicing(bool enable);
	void slicing(GLuint slicingSource);
	void setSlicingPlane(glm::vec3 pos, glm::vec3 dir);
	void setMaterial(float roughness, float metallic);
	void getMaterial(float* roughness, float* metallic);
	void getSettings(float* bundle, bool* _enableSlicing, glm::vec3* slicePos, glm::vec3* sliceDir);
	void activate();
	void deactivate();
	void updateVertexBuffer(GLuint newVertexBuffer, size_t copySize);
	void updateDirectionBuffer(GLuint newDirectionBuffer, size_t copySize);
	int getOriLineBufferSize();
	GLuint getOriFibers();
	void setNowLineBufferSize(int newSize);
	bool isSlicingEnabled();
	void restoreOriginalLines();

private:
	bool isActivated = false;
	std::vector<glm::vec3> tracks;     //stores the original tracks without bundling
	std::vector<uint32_t> trackOffset;
	std::vector<uint32_t> trackSize;     
	std::vector<int>trackId;             //store for each point which track it belongs to
	std::vector<int>isFiberBegin;        //store for each vertex whether it is a begin point of some fiber
	std::vector<int>isFiberEnd;

	bool visible=true;
	GLuint VAO = -1;
	GLuint VBO = -1;
	GLuint IBO = -1;

	//For line mode drawing
	GLuint VAOLines = -1;   
	GLuint VBOLines = -1;   
	GLuint DBOLines = -1;   //lineDirection
	int oriLineBufferSize=0;
	int nowLineBufferSize = 0;

	void loadTracksFromTCK(std::string path);
	std::vector<glm::vec3> readTCK(const std::string& filename);

	//edge-bundling related structures on host 
	float bundle=0;
	AABB aabb;   //bounding box for the instance
	int nIters = 15;  //number of iterations for edge bundling
	float smoothFactor = 0.99;
	float smoothL;
	float relaxFactor = 0.85;

	// CPU-based edge bundling
	//std::vector<float> computeDensity(float p);    //called every time when user-specified parameter changes (the kernel width)
	//std::vector<glm::vec3> advection(std::vector<float>& denseMap,float p);   //called every time when user-specified parameter changes (the kernel width)
	//std::vector<glm::vec3> smoothing(std::vector<glm::vec3>& newTracks);         //called every time when user-specified parameter changes (the kernel width)

	//helper functions
	void trackToLinePass(GLuint texTrack, GLuint texLines);
	void transferDataGPU(GLuint srcBuffer, GLuint dstBuffer, size_t copySize);

	//textures used on GPU
	GLuint texOriFibers;  //persistently store the original fibers, never be modified
	GLuint texTempTracks;
	GLuint texUpdatedTracks;
	GLuint texSmoothedTracks;
	GLuint texRelaxedTracks;
	GLuint texIsFiberBegin;
	GLuint texIsFiberEnd;
	GLuint texTrackId;
	GLuint texRelaxedLines;

	GLuint texTempDirections;
	GLuint texUpdatedDirections;
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
	std::shared_ptr<ComputeShader> slicingShader;
	std::shared_ptr<ComputeShader> trackToLinesShader;

	//DEBUG
	std::vector<Tube>tubes;
	std::vector<glm::vec3>directions;
	void updateTubes(std::vector<glm::vec3>& currentTracks);


	void reduceFibers();
};