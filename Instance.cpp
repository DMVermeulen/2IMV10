#include"Instance.h"
#include<fstream>
#include"glm/glm.hpp"
#include <glm/gtc/constants.hpp>
#include<iostream>
#include <algorithm>
#include <chrono>

#define INFINITE 999999999

Instance::Instance(
	std::string path,
	std::shared_ptr<ComputeShader> _slicingShader,
	std::shared_ptr<ComputeShader> _trackToLinesShader
):  slicingShader(_slicingShader),
	trackToLinesShader(_trackToLinesShader)
{
	loadTracksFromTCK(path);
	//reduceFibers(); //only used for performance evaluation
	initAddition();
	updateTubes(tracks);
}

void Instance::reduceFibers() {
	int interval = trackOffset.size() / 10;
	int scale = 8;
	int nFibers = scale * interval;
	std::vector<glm::vec3> subTracks(tracks.begin(), tracks.begin() + trackOffset.at(nFibers));
	std::vector<uint32_t> subTrackOffset(trackOffset.begin(), trackOffset.begin() + nFibers);
	std::vector<uint32_t> subTrackSize(trackSize.begin(), trackSize.begin() + nFibers);
	
	tracks = subTracks;
	trackOffset = subTrackOffset;
	trackSize = subTrackSize;
}

//explicitly destroy opengl objects
Instance::~Instance() {
	destroyTextures();
}

std::vector<glm::vec3> Instance::readTCK(const std::string& filename) {
	std::ifstream file(filename, std::ios::binary);
	// Read track data from file
	if (!file.is_open()) {
		std::runtime_error("failed to opening file!");
	}

	// Read the offset
	int offset = 0;
	bool found = false;
	std::string line;
	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string field;
		if (iss >> field && field == "file:") {
			if (iss >> field && iss >> offset) {
				found = true;
				break;
			}
		}
	}

	// Seek to the specified offset
	file.seekg(offset, std::ios::beg);

	std::vector<glm::vec3> nowTracks;
	//std::vector<glm::vec3> currentStreamline;
	int count = 0;
	int debug = 0;
	trackOffset.push_back(0);
	int sample = 1;
	glm::vec3 center(0);
	while (!file.eof()) {
		float x, y, z;
		file.read(reinterpret_cast<char*>(&x), sizeof(float));
		file.read(reinterpret_cast<char*>(&y), sizeof(float));
		file.read(reinterpret_cast<char*>(&z), sizeof(float));

		// Check if x is NaN, indicating the start of a new streamline
		if (std::isnan(x)) {
			if (debug % sample != 0) {
				debug++;
				continue;
			}
			if (0 != count) {
				//tracks.push_back(currentStreamline);
				//currentStreamline.clear();
				trackOffset.push_back(nowTracks.size());
				trackSize.push_back(count);
				count = 0;
				debug++;
			}
		}
		else if (std::isinf(x)) {
			// Check if x is Inf, indicating the end of the file
			break;
		}
		else {
			// Add the point to the current streamline
			//currentStreamline.push_back(glm::vec3(x, y, z));
			if (debug % sample != 0) {
				continue;
			}
			count++;
			nowTracks.push_back(glm::vec3(x, y, z));
			center += glm::vec3(x, y, z);
		}
	}
	trackOffset.pop_back();
	file.close();

	center /= nowTracks.size();
	center += glm::vec3(0);
	//transform the model to world center
	for (int i = 0; i < nowTracks.size();i++) {
		nowTracks[i] -= center;
	}
	center = glm::vec3(0);
	for (glm::vec3 &point : nowTracks) {
		center += point;
	}
	center /= nowTracks.size();
	return nowTracks;
}

//Input: file
//Output: tracks
void Instance::loadTracksFromTCK(std::string path) {
	tracks = readTCK(path);
}

const std::vector<glm::vec3>& Instance::getTracks() {
	return tracks;
}

const std::vector<uint32_t>& Instance::getTrackOffset() {
	return trackOffset;
}

const std::vector<uint32_t>& Instance::getTrackSize() {
	return trackSize;
}

//initialize additional information
void Instance::initAddition() {
	isFiberBegin.resize(tracks.size());
	isFiberEnd.resize(tracks.size());
	trackId.resize(tracks.size());
	for (int i = 0; i < trackOffset.size(); i++) {
		isFiberBegin[trackOffset[i]] = 1;
		isFiberEnd[trackOffset[i]+trackSize[i]-1] = 1;
	}
	for (int i = 0; i < trackOffset.size(); i++) {
		for (int k = 0; k < trackSize[i]; k++) {
			trackId[trackOffset[i] + k] = i;
		}
	}
	oriLineBufferSize = 2 * tracks.size() - 2 * trackOffset.size();
	nowLineBufferSize = oriLineBufferSize;

	//calculate AABB
	aabb.minPos = glm::vec3(INFINITE, INFINITE, INFINITE);
	aabb.maxPos = -glm::vec3(INFINITE, INFINITE, INFINITE);
	for (int i = 0; i < tracks.size(); i++) {
		aabb.minPos.x = std::min(aabb.minPos.x, tracks[i].x);
		aabb.minPos.y = std::min(aabb.minPos.y, tracks[i].y);
		aabb.minPos.z = std::min(aabb.minPos.z, tracks[i].z);

		aabb.maxPos.x = std::max(aabb.maxPos.x, tracks[i].x);
		aabb.maxPos.y = std::max(aabb.maxPos.y, tracks[i].y);
		aabb.maxPos.z = std::max(aabb.maxPos.z, tracks[i].z);
	}
}


void Instance::setVisible(bool _visible) {
	visible = _visible;
}

void Instance::drawLineMode(float lineWidth,int pointCount) {
	glBindVertexArray(VAOLines);
	glLineWidth(lineWidth);
	glDrawArrays(GL_LINES, 0, pointCount);

	glBindVertexArray(0);
}

int Instance::getOriLineBufferSize() {
	return oriLineBufferSize;
}

void Instance::setNowLineBufferSize(int newSize) {
	nowLineBufferSize = newSize;
}

//map points in tracks to lines (double repeat the interior points in a track)
void Instance::trackToLinePass(GLuint texTracks, GLuint texLines) {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texTracks);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, texLines);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, texIsFiberBegin);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, texIsFiberEnd);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, texTrackId);

	trackToLinesShader->use();
	trackToLinesShader->setInt("totalSize", tracks.size());

	glDispatchCompute(1 + (unsigned int)tracks.size() / 128, 1, 1);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void Instance::transferDataGPU(GLuint srcBuffer, GLuint dstBuffer, size_t copySize) {
	glBindBuffer(GL_ARRAY_BUFFER,srcBuffer);
	glBindBuffer(GL_COPY_WRITE_BUFFER, dstBuffer);

	// Transfer data from buffer 1 to buffer 2
	glCopyBufferSubData(GL_ARRAY_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, copySize);

	//For synchronization
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// Unbind buffers
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
}

glm::vec3 Instance::getAABBMin() {
	return aabb.minPos;
}

void Instance::slicing(GLuint slicingSource) {
	if (!enableSlicling)
		return;
	if (glm::dot(slicingDir, slicingDir) < 1e-3)
		return;
	if (glm::dot(slicingPos, slicingPos) < 1e-2)
		return;

	//slight modification for corner cases
	glm::vec3 pos = slicingPos + glm::vec3(0.1);
	glm::vec3 dir = slicingDir;

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, slicingSource);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, VBOLines);

	glm::vec3 _slicingPos = aabb.minPos+(aabb.maxPos-aabb.minPos)*pos;
	glm::vec3 _slicingDir = dir;

	slicingShader->use();
	slicingShader->setInt("totalSize", nowLineBufferSize/2);
	slicingShader->setVec3("pos", _slicingPos);
	slicingShader->setVec3("dir", _slicingDir);

	glDispatchCompute(1 + (unsigned int)(nowLineBufferSize / 2) / 128, 1, 1);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void Instance::setSlicingPlane(glm::vec3 pos, glm::vec3 dir) {
	slicingPos = pos;
	slicingDir = dir;
}

void Instance::setEnableSlicing(bool enable) {
	enableSlicling = enable;
	//enableSlicling = !enableSlicling;
	//slicingPos = pos;
	//slicingDir = dir;
	//if (enableSlicling)
	//	slicing();
	//else
	//	transferDataGPU(texRelaxedLines, VBOLines, lineBufferSize*3*sizeof(float));
}

void Instance::setMaterial(float _roughness, float _metallic) {
	roughness = _roughness;
	metallic = _metallic;
}
void Instance::getMaterial(float* _roughness, float* _metallic) {
	*_roughness = roughness;
	*_metallic = metallic;
}

void Instance::activate() {
	createTextures();
	//if(bundle>0)
	// edgeBundlingGPU(bundle);
	isActivated = true;
}

void Instance::deactivate() {
	destroyTextures();
	isActivated = false;
}

void Instance::destroyTextures() {
	GLuint buffersToDelete[] = {
    VBOLines,
    DBOLines,
    texTempTracks,
    texUpdatedTracks,
    texSmoothedTracks,
    texRelaxedTracks,
    texTempDirections,
    texUpdatedDirections,
    texIsFiberBegin,
	texIsFiberEnd,
	texTrackId,
	texRelaxedLines
	};
	glDeleteBuffers(12, buffersToDelete);

	glDeleteVertexArrays(1,&VAOLines);
}

void Instance::createTextures() {
	glGenVertexArrays(1, &VAOLines);
	glGenBuffers(1, &VBOLines);
	glGenBuffers(1, &DBOLines);

	glBindVertexArray(VAOLines);
	glBindBuffer(GL_ARRAY_BUFFER, VBOLines);
	glBufferData(GL_ARRAY_BUFFER, size_t(oriLineBufferSize) * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, DBOLines);
	glBufferData(GL_ARRAY_BUFFER, size_t(oriLineBufferSize) * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	//original fibers
	glGenBuffers(1, &texOriFibers);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texOriFibers);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size_t(oriLineBufferSize) * 3 * sizeof(float), NULL, GL_STATIC_DRAW);

	//temp tracks
	glGenBuffers(1, &texTempTracks);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texTempTracks);
	glBufferData(GL_SHADER_STORAGE_BUFFER, tracks.size() * 3 * sizeof(float), NULL, GL_STATIC_DRAW);

	//isFiberBegin
	glGenBuffers(1, &texIsFiberBegin);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texIsFiberBegin);
	glBufferData(GL_SHADER_STORAGE_BUFFER, isFiberBegin.size() * sizeof(int), isFiberBegin.data(), GL_STATIC_DRAW);

	//isFiberEnd
	glGenBuffers(1, &texIsFiberEnd);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texIsFiberEnd);
	glBufferData(GL_SHADER_STORAGE_BUFFER, isFiberEnd.size() * sizeof(int), isFiberEnd.data(), GL_STATIC_DRAW);

	//trackId
	glGenBuffers(1, &texTrackId);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texTrackId);
	glBufferData(GL_SHADER_STORAGE_BUFFER, trackId.size() * sizeof(int), trackId.data(), GL_STATIC_DRAW);

	//init VBO by original tracks (using a compute shader)
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texTempTracks);
	glBufferData(GL_SHADER_STORAGE_BUFFER, tracks.size() * 3 * sizeof(float), tracks.data(), GL_STATIC_DRAW);
	trackToLinePass(texTempTracks,VBOLines);

	transferDataGPU(VBOLines, texOriFibers, size_t(oriLineBufferSize) * 3 * sizeof(float));

	//init texRelaxedlines for slicing
	//transferDataGPU(VBOLines, texRelaxedLines, size_t(oriLineBufferSize) * 3 * sizeof(float));

	//init DBO (using a compute shader)
	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, texRelaxedTracks);
	//glBufferData(GL_SHADER_STORAGE_BUFFER, tracks.size() * 3 * sizeof(float), tracks.data(), GL_STATIC_DRAW);
	//updateDirectionPass();
	//trackToLinePass(texUpdatedDirections, DBOLines);

	//DEBUG
	glBindBuffer(GL_ARRAY_BUFFER, DBOLines);
	glBufferData(GL_ARRAY_BUFFER, size_t(oriLineBufferSize) * 3 * sizeof(float), directions.data(), GL_STATIC_DRAW);
}

//DEBUG
void Instance::updateTubes(std::vector<glm::vec3>& currentTracks) {
	tubes.clear();
	int offset = 0;
	for (int i = 0; i < trackOffset.size(); i++) {
		for (int j = trackOffset[i]; j < trackOffset[i] + trackSize[i] - 1; j++) {
			tubes.push_back(Tube{ currentTracks.at(j) ,currentTracks.at(j + 1) });
			float leng = glm::dot(currentTracks[j] - currentTracks[j + 1], currentTracks[j] - currentTracks[j + 1]);
		}
	}

	for (int i = 0; i < tubes.size(); i++) {
		glm::vec3 dir = glm::normalize(tubes[i].p1 - tubes[i].p2);
		directions.push_back(dir);
		directions.push_back(dir);
	}
}

void Instance::getSettings(float* _bundle, bool* _enableSlicing, glm::vec3* _slicePos, glm::vec3* _sliceDir) {
	*_bundle = bundle;
	*_enableSlicing = enableSlicling;
	*_slicePos = slicingPos;
	*_sliceDir = slicingDir;
}

bool Instance::isSlicingEnabled() {
	return enableSlicling;
}

void Instance::updateVertexBuffer(GLuint newVertexBuffer, size_t copySize) {
	glBindBuffer(GL_ARRAY_BUFFER, VBOLines);
	glBufferData(GL_ARRAY_BUFFER, copySize, NULL, GL_DYNAMIC_DRAW);
	transferDataGPU(newVertexBuffer, VBOLines, copySize);
}

void Instance::updateDirectionBuffer(GLuint newDirectionBuffer, size_t copySize) {
	glBindBuffer(GL_ARRAY_BUFFER, DBOLines);
	glBufferData(GL_ARRAY_BUFFER, copySize, NULL, GL_DYNAMIC_DRAW);
	transferDataGPU(newDirectionBuffer, DBOLines, copySize);
}

//restore the original fibers when either slicing or bundling is set to disabled
void Instance::restoreOriginalLines() {
	glBindBuffer(GL_ARRAY_BUFFER, VBOLines);
	glBufferData(GL_ARRAY_BUFFER, oriLineBufferSize * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, DBOLines);
	glBufferData(GL_ARRAY_BUFFER, oriLineBufferSize * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, texTempTracks);
	//glBufferData(GL_SHADER_STORAGE_BUFFER, tracks.size() * 3 * sizeof(float), tracks.data(), GL_STATIC_DRAW);
	//trackToLinePass(texTempTracks, VBOLines);

	transferDataGPU(texOriFibers, VBOLines, oriLineBufferSize * 3 * sizeof(float));

	glBindBuffer(GL_ARRAY_BUFFER, DBOLines);
	glBufferData(GL_ARRAY_BUFFER, oriLineBufferSize * 3 * sizeof(float), directions.data(), GL_DYNAMIC_DRAW);

	nowLineBufferSize = oriLineBufferSize;
}

GLuint Instance::getOriFibers() {
	return texOriFibers;
}

void Instance::setBundleValue(float _bundle) {
	bundle = _bundle;
}


float Instance::getBundleValue() {
	return bundle;
}


