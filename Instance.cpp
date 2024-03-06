#include"Instance.h"
#include<fstream>
#include"glm/glm.hpp"
#include <glm/gtc/constants.hpp>
#include<iostream>
#include <algorithm>

#define INFINITE 999999999

Instance::Instance(std::string path, float radius, int nTris)
	:denseEstimationShader("D:/Projects/FiberVisualization/shaders/denseEstimation.cs")
	,advectionShader("D:/Projects/FiberVisualization/shaders/advection.cs")
    ,voxelCountShader("D:/Projects/FiberVisualization/shaders/voxelCount.cs")
    ,smoothShader("D:/Projects/FiberVisualization/shaders/smooth.cs")
	, relaxShader("D:/Projects/FiberVisualization/shaders/relaxation.cs") 
	, updateDirectionShader("D:/Projects/FiberVisualization/shaders/updateDirections.cs") 
	, updateNormalShader("D:/Projects/FiberVisualization/shaders/updateNormals.cs")
	, forceConsecutiveShader("D:/Projects/FiberVisualization/shaders/forceConsecutive.cs") {
	loadTracksFromTCK(path);
	//trackResampling();
	updateTubes(tracks);
	initLineDirections();
	initLineNormals();

	//Triangle mode
	//updateTriangles(radius, nTris);
	//updateVertexIndiceBuffer();
	
	//Line mode
	initVertexBufferLineMode();
	spaceVoxelization();
	initTextures();
}

Instance::~Instance() {

}

void Instance::initTextures() {
	//Currently we implement all textures as 1-D buffer
	////original tracks
	//glGenBuffers(1, &texOriTracks);
	//glBindBuffer(GL_TEXTURE_BUFFER, texOriTracks);
	//glBufferData(GL_TEXTURE_BUFFER, tracks.size() * 3 * sizeof(float), tracks.data(), GL_STATIC_DRAW);

	////temp tracks
	//glGenBuffers(1, &texTempTracks);
	//glBindBuffer(GL_TEXTURE_BUFFER, texTempTracks);
	//glBufferData(GL_TEXTURE_BUFFER, tracks.size() * 3 * sizeof(float), NULL, GL_STATIC_DRAW);

	////updated tracks
	//glGenBuffers(1, &texUpdatedTracks);
	//glBindBuffer(GL_TEXTURE_BUFFER, texUpdatedTracks);
	//glBufferData(GL_TEXTURE_BUFFER, tracks.size() * 3 * sizeof(float), NULL, GL_STATIC_DRAW);

	//temp tracks
	glGenBuffers(1, &texTempTubes);
	glBindBuffer(GL_TEXTURE_BUFFER, texTempTubes);
	glBufferData(GL_TEXTURE_BUFFER, tubes.size() * 6 * sizeof(float), NULL, GL_STATIC_DRAW);

	//updated tracks
	glGenBuffers(1, &texUpdatedTubes);
	glBindBuffer(GL_TEXTURE_BUFFER, texUpdatedTubes);
	glBufferData(GL_TEXTURE_BUFFER, tubes.size() * 6 * sizeof(float), NULL, GL_STATIC_DRAW);

	//smoothed tracks
	glGenBuffers(1, &texSmoothedTubes);
	glBindBuffer(GL_TEXTURE_BUFFER, texSmoothedTubes);
	glBufferData(GL_TEXTURE_BUFFER, tubes.size() * 6 * sizeof(float), NULL, GL_STATIC_DRAW);

	//relaxed tracks
	glGenBuffers(1, &texRelaxedTubes);
	glBindBuffer(GL_TEXTURE_BUFFER, texRelaxedTubes);
	glBufferData(GL_TEXTURE_BUFFER, tubes.size() * 6 * sizeof(float), NULL, GL_STATIC_DRAW);

	//temp normals
	glGenBuffers(1, &texTempNormals);
	glBindBuffer(GL_TEXTURE_BUFFER, texTempNormals);
	glBufferData(GL_TEXTURE_BUFFER, tubes.size() * 6 * sizeof(float), NULL, GL_STATIC_DRAW);

	//updated normals
	glGenBuffers(1, &texUpdatedNormals);
	glBindBuffer(GL_TEXTURE_BUFFER, texUpdatedNormals);
	glBufferData(GL_TEXTURE_BUFFER, tubes.size() * 6 * sizeof(float), NULL, GL_STATIC_DRAW);

	//temp directions
	glGenBuffers(1, &texTempDirections);
	glBindBuffer(GL_TEXTURE_BUFFER, texTempDirections);
	glBufferData(GL_TEXTURE_BUFFER, tubes.size() * 6 * sizeof(float), NULL, GL_STATIC_DRAW);

	//updated directions
	glGenBuffers(1, &texUpdatedDirections);
	glBindBuffer(GL_TEXTURE_BUFFER, texUpdatedDirections);
	glBufferData(GL_TEXTURE_BUFFER, tubes.size() * 6 * sizeof(float), NULL, GL_STATIC_DRAW);

	//voxelCount (1-D buffer)
	glGenBuffers(1, &texVoxelCount);
	glBindBuffer(GL_TEXTURE_BUFFER, texVoxelCount);
	glBufferData(GL_TEXTURE_BUFFER, totalVoxels * sizeof(uint32_t), NULL, GL_STATIC_READ | GL_STATIC_DRAW);

	//denseMap (1-D buffer)
	glGenBuffers(1, &texDenseMap);
	glBindBuffer(GL_TEXTURE_BUFFER, texDenseMap);
	glBufferData(GL_TEXTURE_BUFFER, totalVoxels * sizeof(float), NULL, GL_STATIC_READ| GL_STATIC_DRAW);

	//isFiberEndpoint
	glGenBuffers(1, &texIsFiberEndpoint);
	glBindBuffer(GL_TEXTURE_BUFFER, texIsFiberEndpoint);
	glBufferData(GL_TEXTURE_BUFFER, isFiberEndpoint.size() * sizeof(int), isFiberEndpoint.data(), GL_STATIC_DRAW);

	//debug
	glGenBuffers(1, &debugUINT);
	glBindBuffer(GL_TEXTURE_BUFFER, debugUINT);
	glBufferData(GL_TEXTURE_BUFFER, totalVoxels * sizeof(uint32_t), NULL, GL_STATIC_DRAW);

	glGenBuffers(1, &debugFLOAT);
	glBindBuffer(GL_TEXTURE_BUFFER, debugFLOAT);
	glBufferData(GL_TEXTURE_BUFFER, 6* tubes.size() * sizeof(float), NULL, GL_STATIC_DRAW);
}

int Instance::getNumberVertices(){
	return vertices.size();
}

int Instance::getNumberIndices() {
	return indices.size();
}

int Instance::getNumberTriangles() {
	return indices.size() / 3;
}

std::vector<Vertex>& Instance::getVertices() {
	return vertices;
}

std::vector<uint32_t>& Instance::getIndices() {
	return indices;
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
	int sample = 20;
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

void Instance::trackResampling() {
	std::vector<float>aveLength;
	for (int i = 0; i < trackOffset.size(); i++) {
		float sum = 0;
		for (int j = trackOffset[i]; j < trackOffset[i] + trackSize[i]-1; j++) {
			float leng = glm::dot(tracks[j] - tracks[j + 1], tracks[j] - tracks[j + 1]);
			sum += sqrt(leng);
		}
		sum /= trackSize[i];
		aveLength.push_back(sum);
	}
	
	std::vector<glm::vec3>resampledTracks;
	for (int i = 0; i < trackOffset.size(); i++) {
		resampledTracks.push_back(tracks[trackOffset[i]]);
		for (int j = trackOffset[i]+1; j < trackOffset[i] + trackSize[i]; j++) {
			glm::vec3 dir = glm::normalize(tracks[j] - tracks[j-1]);
			glm::vec3 newPoint = resampledTracks[j - 1] + aveLength[i]*dir;
			resampledTracks.push_back(newPoint);
		}
	}
	tracks = resampledTracks;
}

//Recreate tubes from new tracks
//Input: current tracks
//Output: updated tubes
void Instance::updateTubes(std::vector<glm::vec3>& currentTracks) {
	tubes.clear();
	int offset = 0;
	for (int i = 0; i < trackOffset.size(); i++) {
		for (int j = trackOffset[i]; j < trackOffset[i]+trackSize[i]-1; j++) {
			tubes.push_back(Tube{ currentTracks[j] ,currentTracks[j + 1]});
			float leng = glm::dot(currentTracks[j] - currentTracks[j + 1], currentTracks[j] - currentTracks[j + 1]);
			if (j == trackOffset[i]) {
				isFiberEndpoint.push_back(1);
			}
			else {
				isFiberEndpoint.push_back(0);
				isFiberEndpoint.push_back(0);
			}
		}
		isFiberEndpoint.push_back(1);
	}
}

//Output: updated vertices and indices
void Instance::updateTriangles(float radius, int nTris) {
	vertices.clear();
	indices.clear();
	float delta = 2 * glm::pi<float>() / nTris;
	int offset = 0;
	for (int i = 0; i < tubes.size(); i++) {
		Tube tube = tubes[i];
		glm::vec3 up = glm::vec3(0, 0, 1);
		glm::vec3 ez = glm::normalize(tube.p2 - tube.p1);
		glm::vec3 ey = cross(up, ez);
		glm::vec3 ex = cross(ez, ey);

		glm::mat3 R = glm::mat3(ex, ey, ez);
		glm::vec3 T = tube.p1;
		float leng = sqrt(glm::dot(tubes[i].p2 - tubes[i].p1, tubes[i].p2 - tubes[i].p1));
		for (int k = 0; k < nTris; k++) {
			float theta = delta * k;
			vertices.push_back(Vertex{ glm::vec4(R*glm::vec3(radius*glm::cos(theta),radius*glm::sin(theta),leng) + T,0) });
		}
		for (int k = 0; k < nTris; k++) {
			float theta = delta * k;
			vertices.push_back(Vertex{ glm::vec4(R*glm::vec3(radius*glm::cos(theta),radius*glm::sin(theta),0) + T,0) });
		}
		for (int k = 0; k < nTris; k++) {
			indices.push_back(offset + k);
			indices.push_back(offset + nTris + k);
			indices.push_back(offset + (k + 1) % nTris);

			indices.push_back(offset + (k + 1) % nTris);
			indices.push_back(offset + nTris + k);
			indices.push_back(offset + nTris + (k + 1) % nTris);
		}
		offset += 2 * nTris;
	}

	////debug
	//vertices = { Vertex{glm::vec3(-0.5f, -0.5f, -10)},Vertex{glm::vec3(0.5f, -0.5f, -10)},Vertex{glm::vec3(0.5f, 0.5f, -10)} };
	//indices = { 0,1,2 };
}

void Instance::setVisible(bool _visible) {
	visible = _visible;
}

void Instance::updateVertexIndiceBuffer() {
	if (-1!=VAO) {
	    //destroy old buffers 
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &IBO);
	}
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &IBO);

	glBindVertexArray(VAO);

	//VBO stores vertex positions, possibly extended to store more attributes
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	//IBO stores indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);
}

void Instance::drawLineMode(float lineWidth) {
	glBindVertexArray(VAOLines);
	glLineWidth(lineWidth);
	glDrawArrays(GL_LINES, 0, 2*tubes.size());

	glBindVertexArray(0);
}

void Instance::recreateMeshNewRadius(float radius, int nTris) {
	updateTriangles(radius, nTris);
	//load new data to gpu
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * vertices.size(), vertices.data());

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Instance::recreateMeshNewNTris(float radius, int nTris) {
	//create meshes by new parameters
	updateTriangles(radius, nTris);
	//recreate VBO and IBO, load new data
	updateVertexIndiceBuffer();
}

void Instance::edgeBundling(float p, float radius, int nTris){
	resampling();
	std::vector<float>denseMap = computeDensity(p);
	std::vector<glm::vec3> newTracks = advection(denseMap,p);
	std::vector<glm::vec3> smoothTracks = smoothing(newTracks);
	updateTubes(smoothTracks);
	updateTriangles(radius,nTris);
	updateVertexIndiceBuffer();
}

//Assign points to voxels
//Input: tracks
//Output: voxelAssignment
void Instance::spaceVoxelization() {
	//calculate AABB first
	aabb.minPos = glm::vec3(INFINITE, INFINITE, INFINITE);
	aabb.maxPos= -glm::vec3(INFINITE, INFINITE, INFINITE);
	for (int i = 0; i < tracks.size(); i++) {
		aabb.minPos.x = std::min(aabb.minPos.x, tracks[i].x);
		aabb.minPos.y = std::min(aabb.minPos.y, tracks[i].y);
		aabb.minPos.z = std::min(aabb.minPos.z, tracks[i].z);

		aabb.maxPos.x = std::max(aabb.maxPos.x, tracks[i].x);
		aabb.maxPos.y = std::max(aabb.maxPos.y, tracks[i].y);
		aabb.maxPos.z = std::max(aabb.maxPos.z, tracks[i].z);
	}
	glm::vec3 delta = aabb.maxPos - aabb.minPos;
	voxelUnitSize = delta.z / nVoxels_Z;
	nVoxels_X = (uint32_t)(delta.x / voxelUnitSize)+1;
	nVoxels_Y = (uint32_t)(delta.y / voxelUnitSize)+1;

	totalVoxels = nVoxels_X * nVoxels_Y*nVoxels_Z;

	//std::vector<std::vector<int>> temp;
	//temp.resize(nVoxels_X*nVoxels_Y*nVoxels_Z);
	////assign vertices to voxels (index order: z,x,y)
	//for (int i = 0; i < tracks.size();i++) {
	//	glm::vec3 &point = tracks[i];
	//	glm::vec3 deltaP = point - aabb.minPos;
	//	uint32_t level_X = std::min(float(nVoxels_X - 1), deltaP.x / voxelUnitSize);
	//	uint32_t level_Y = std::min(float(nVoxels_Y - 1), deltaP.y / voxelUnitSize);
	//	uint32_t level_Z = std::min(float(nVoxels_Z - 1), deltaP.z / voxelUnitSize);
	//	uint32_t index = nVoxels_X * nVoxels_Y*level_Z + nVoxels_Y * level_X + level_Y;
	//	if (temp[index].empty())
	//		temp[index].resize(0);
	//	temp[index].push_back(i);
	//}
	//int offset = 0;
	//for (int i = 0; i < temp.size(); i++) {
	//	for (int k = 0; k < temp[i].size(); k++) {
	//		voxelAssignment.push_back(temp[i][k]);
	//	}
	//	voxelOffset.push_back(offset);
	//	voxelSize.push_back(temp[i].size());
	//	offset += temp[i].size();
	//}

	//assign vertices to voxels (index order: z,x,y)
	//voxelCount.resize(nVoxels_X*nVoxels_Y*nVoxels_Z);
	//std::fill(voxelCount.begin(), voxelCount.end(), 0);
	//for (int i = 0; i < tracks.size(); i++) {
	//	glm::vec3 &point = tracks[i];
	//	glm::vec3 deltaP = point - aabb.minPos;
	//	uint32_t level_X = std::min(float(nVoxels_X - 1), deltaP.x / voxelUnitSize);
	//	uint32_t level_Y = std::min(float(nVoxels_Y - 1), deltaP.y / voxelUnitSize);
	//	uint32_t level_Z = std::min(float(nVoxels_Z - 1), deltaP.z / voxelUnitSize);
	//	uint32_t index = nVoxels_X * nVoxels_Y*level_Z + nVoxels_X * level_Y + level_X;
	//	voxelCount[index]++;
	//}

	//int cnt = 0;
	//for (int i = 0; i < voxelCount.size(); i++) {
	//	if (voxelCount[i] == 0)
	//		cnt++;
	//}
	//cnt++;
}

void Instance::resampling() {

}

//Input: voxelAssignment, parameter Pr
//Output: 3D dense map of size nVertices
std::vector<float> Instance::computeDensity(float p) {
	std::vector<float>denseMap;
	//denseMap.resize(tracks.size());
	//int kernelWidth = p * nVoxels_Z;
	//for (int i = 0; i < denseMap.size(); i++) {
	//	if(i%100==0)
	//	  std::cout << i << std::endl;
	//	float dense = 0;
	//	glm::vec3 point = tracks.at(i);
	//	glm::vec3 deltaP = point - aabb.minPos;
	//	int X = deltaP.x / voxelUnitSize;
	//	int Y = deltaP.y / voxelUnitSize;
	//	int Z = deltaP.z / voxelUnitSize;
	//	for (int dx = - kernelWidth; dx < kernelWidth; dx++) {
	//		for (int dy = - kernelWidth; dy < kernelWidth; dy++) {
	//			for (int dz = - kernelWidth; dz < kernelWidth; dz++) {
	//				int nx = X+dx;
	//				nx = std::max(nx, 0);
	//				nx = std::min(nx, int(nVoxels_X-1));
	//				int ny = Y + dy;
	//				ny = std::max(ny, 0);
	//				ny = std::min(ny, int(nVoxels_Y-1));
	//				int nz = Z + dz;
	//				nz = std::max(nz, 0);
	//				nz = std::min(nz, int(nVoxels_Z-1));

	//				uint32_t index = nVoxels_X * nVoxels_Y*nz + nVoxels_Y * nx + ny;
	//				index = std::min(int(index), int(voxelOffset.size()));
	//				for (int k = voxelOffset[index]; k < voxelOffset[index] + voxelSize[index]; k++) {
	//					glm::vec3 pointB = tracks[voxelAssignment[k]];
	//					glm::vec3 diff = pointB - point;
	//					float dot = glm::dot(diff, diff);
	//					float width = kernelWidth * voxelUnitSize;
	//					//Apply parabolic kernel
	//					dense += (3 / (4 * width))*(1 - dot / (width*width));
	//				}
	//			}
	//		}
	//	}
	//	denseMap[i] = dense;
	//}

	denseMap.resize(nVoxels_X*nVoxels_Y*nVoxels_Z);
	int kernelR = p * nVoxels_Z;
	for (int i = 0; i < denseMap.size(); i++) {
		if(i%1000==0)
		std::cout << i << std::endl;
		float dense = 0;
		int Z = i / (nVoxels_X*nVoxels_Y);
		int mod= i % (nVoxels_X*nVoxels_Y);
		int X = mod % nVoxels_X;
		int Y = mod / nVoxels_X;
		for (int dx = -kernelR; dx < kernelR; dx++) {
			for (int dy = -kernelR; dy < kernelR; dy++) {
				for (int dz = -kernelR; dz < kernelR; dz++) {
					int nx = X + dx;
					nx = std::max(nx, 0);
					nx = std::min(nx, int(nVoxels_X - 1));
					int ny = Y + dy;
					ny = std::max(ny, 0);
					ny = std::min(ny, int(nVoxels_Y - 1));
					int nz = Z + dz;
					nz = std::max(nz, 0);
					nz = std::min(nz, int(nVoxels_Z - 1));
					glm::vec3 diff = voxelUnitSize * (glm::vec3(nx, ny, nz) - glm::vec3(X, Y, Z));
					float dot = glm::dot(diff, diff);
					float PR2 = kernelR * voxelUnitSize*kernelR * voxelUnitSize;
					if (dot > PR2)
						continue;

					uint32_t index = nVoxels_X * nVoxels_Y*nz + nVoxels_X * ny + nx;
					index = std::min(int(index), int(voxelCount.size()));
					int pointCnt = voxelCount[index];
					
					dense += pointCnt * (1 - dot/PR2);
				}
			}
		}
		denseMap[i] = dense;
		if (i == 920)
			std::cout << dense;
	}

	return denseMap;
}

//Input: dense map
//Output: updated tracks
std::vector<glm::vec3> Instance::advection(std::vector<float>& denseMap, float p) {
	std::vector<glm::vec3> newTracks;
	int kernelR = p * nVoxels_Z;
	int R2 = kernelR * kernelR;
	int kernelWidth = 2 * kernelR + 1;
	//for (int i = 0; i < tracks.size(); i++) {
	//	glm::vec3 grad = glm::vec3(0);
	//	glm::vec3 point = tracks.at(i);
	//	glm::vec3 deltaP = point - aabb.minPos;
	//	int X = deltaP.x / voxelUnitSize;
	//	int Y = deltaP.y / voxelUnitSize;
	//	int Z = deltaP.z / voxelUnitSize;
	//	int count = 0;
	//	for (int dx = -kernelWidth; dx < kernelWidth; dx++) {
	//		for (int dy = -kernelWidth; dy < kernelWidth; dy++) {
	//			for (int dz = -kernelWidth; dz < kernelWidth; dz++) {
	//				int nx = X + dx;
	//				nx = std::max(nx, 0);
	//				nx = std::min(nx, int(nVoxels_X-1));
	//				int ny = Y + dy;
	//				ny = std::max(ny, 0);
	//				ny = std::min(ny, int(nVoxels_Y-1));
	//				int nz = Z + dz;
	//				nz = std::max(nz, 0);
	//				nz = std::min(nz, int(nVoxels_Z-1));

	//				uint32_t index = nVoxels_X * nVoxels_Y*nz + nVoxels_Y * nx + ny;
	//				index = std::min(int(index), int(voxelOffset.size()));
	//				for (int k = voxelOffset[index]; k < voxelOffset[index] + voxelSize[index]; k++) {
	//					glm::vec3 pointB = tracks[voxelAssignment[k]];
	//					glm::vec3 diff = pointB - point;
	//					float dot = glm::dot(diff, diff);
	//					float width = kernelWidth * voxelUnitSize;
	//					grad += ((denseMap[voxelAssignment[k]]-denseMap[i]) / dot)*diff;
	//					count++;
	//				}
	//			}
	//		}
	//	}
	//	grad /= count;
	//	glm::vec3 newPoint = tracks[i] + voxelUnitSize * glm::normalize(grad);
	//	newTracks.push_back(newPoint);
	//}

	for (int i = 0; i < tracks.size(); i++) {
		if (i % 5 == 0)
			std::cout << i << std::endl;
		glm::vec3 grad = glm::vec3(0);
		glm::vec3 point = tracks.at(i);
		glm::vec3 deltaP = point - aabb.minPos;
		int X = deltaP.x / voxelUnitSize;
		int Y = deltaP.y / voxelUnitSize;
		int Z = deltaP.z / voxelUnitSize;
		X = std::min(X, int(nVoxels_X - 1));
		Y = std::min(Y, int(nVoxels_Y - 1));
		Z = std::min(Z, int(nVoxels_Z - 1));
		uint32_t indexA= nVoxels_X * nVoxels_Y*Z + nVoxels_X * Y + X;
		for (int dx = -kernelR; dx < kernelR; dx++) {
			for (int dy = -kernelR; dy < kernelR; dy++) {
				for (int dz = -kernelR; dz < kernelR; dz++) {
					int nx = X + dx;
					nx = std::max(nx, 0);
					nx = std::min(nx, int(nVoxels_X - 1));
					int ny = Y + dy;
					ny = std::max(ny, 0);
					ny = std::min(ny, int(nVoxels_Y - 1));
					int nz = Z + dz;
					nz = std::max(nz, 0);
					nz = std::min(nz, int(nVoxels_Z - 1));

					glm::vec3 dir = glm::vec3(dx, dy, dz);
					float diffPos = glm::dot(dir, dir) / R2;
					if (diffPos < 1e-5)
						continue;
					if (diffPos > 1)
						continue;
					uint32_t indexB = nVoxels_X * nVoxels_Y*nz + nVoxels_X * ny + nx;
					indexB = std::min(int(indexB), int(denseMap.size()));
					float diffDense = denseMap[indexB] - denseMap[indexA];

					glm::vec3 old = grad;
					grad += glm::normalize(dir)*diffDense*expf(-diffPos);
		
					//debug
					if (std::isnan(grad.x)&& !std::isnan(old.x)) {
						std::cout << glm::normalize(dir).x <<" "<<diffDense << " "<< expf(-diffPos)<< std::endl;
					}
				}
			}
		}
		grad = grad/float(kernelWidth * kernelWidth*kernelWidth);
		glm::vec3 newPoint;
		if (glm::dot(grad, grad) < 1e-5)
			newPoint = tracks[i];
		else
		    newPoint = tracks[i] + kernelR*voxelUnitSize * glm::normalize(grad);
		newTracks.push_back(newPoint);
	}
	//std::cout << "done" << std::endl;
	//std::cout << newTracks[22].x;
	return newTracks;
}

//Input: tracks
//Output: updated tracks
std::vector<glm::vec3> Instance::smoothing(std::vector<glm::vec3>& newTracks) {
	return newTracks;
}

void Instance::edgeBundlingGPU(float p, float radius, int nTris) {
	//repeat bundling for several iterations

    //init texTempTubes with originaltubes
	glBindBuffer(GL_TEXTURE_BUFFER, texTempTubes);
	glBufferData(GL_TEXTURE_BUFFER, tubes.size() * 6 * sizeof(float), tubes.data(), GL_STATIC_DRAW);
	//init texTempNormals with original normals
	glBindBuffer(GL_TEXTURE_BUFFER, texTempNormals);
	glBufferData(GL_TEXTURE_BUFFER, tubes.size() * 6 * sizeof(float), normals.data(), GL_STATIC_DRAW);
	//init texTempNormals with original directions
	glBindBuffer(GL_TEXTURE_BUFFER, texTempDirections);
	glBufferData(GL_TEXTURE_BUFFER, tubes.size() * 6 * sizeof(float), directions.data(), GL_STATIC_DRAW);

	for (int i = 0; i < nIters; i++) {
		//clear voxelCount at the beginning of each iteration
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, texVoxelCount);
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, nullptr);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		voxelCountPass();
		denseEstimationPass(p);
		advectionPass(p);
		smoothPass(p);
		//re-transfer original tracks to texTempTubes, for relaxation pass
		glBindBuffer(GL_TEXTURE_BUFFER, texTempTubes);
		glBufferData(GL_TEXTURE_BUFFER, tubes.size() * 6 * sizeof(float), tubes.data(), GL_STATIC_DRAW);
		relaxationPass();
		forceConsecutivePass();
		updateDirectionPass();
		updateNormalPass();

		transferDataGPU(texSmoothedTubes, texTempTubes, tubes.size() * 6 * sizeof(float));  //copy data from resulting updated to temp for next iteration
		transferDataGPU(texUpdatedDirections, texTempDirections, tubes.size() * 6 * sizeof(float));
		transferDataGPU(texUpdatedNormals, texTempNormals, tubes.size() * 6 * sizeof(float));
	}
	//line mode
	transferDataGPU(texRelaxedTubes, VBOLines, tubes.size() * 6 * sizeof(float));  //copy data from smoothed to Vertex buffer
	transferDataGPU(texUpdatedDirections, DBOLines, tubes.size() * 6 * sizeof(float)); 
	transferDataGPU(texUpdatedNormals, NBOLines, tubes.size() * 6 * sizeof(float));  

	//triangle mode
	//updateTubes(newTracks);
	//updateTriangles(radius, nTris);
	//updateVertexIndiceBuffer();
}

//tubes->voxelCount
void Instance::voxelCountPass() {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texTempTubes);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, texVoxelCount);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, debugFLOAT);

	voxelCountShader.use();
	voxelCountShader.setInt("totalSize", 2 * tubes.size());
	voxelCountShader.setInt("nVoxels_X", nVoxels_X);
	voxelCountShader.setInt("nVoxels_Y", nVoxels_Y);
	voxelCountShader.setInt("nVoxels_Z", nVoxels_Z);
	voxelCountShader.setVec3("aabbMin", aabb.minPos);
	voxelCountShader.setFloat("voxelUnitSize", voxelUnitSize);

	glDispatchCompute(1 + (unsigned int)2*tubes.size() / 128, 1, 1);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	//glBindBuffer(GL_ARRAY_BUFFER, texVoxelCount);
	//uint32_t * debugData = (uint32_t*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
	//int cnt = 0;
	//for (int i = 0; i < totalVoxels; i++) {
	//	if (debugData[i] != 0)
	//		std::cout << debugData[i];
	//}

	//glBindBuffer(GL_ARRAY_BUFFER, debugFLOAT);
 //   float * debugData = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
 //   for (int i = 0; i < totalVoxels; i++) {
	//   std::cout << debugData[i];
 //    }

	////debug
	//glBindBuffer(GL_TEXTURE_BUFFER, texVoxelCount);
	//glBufferData(GL_TEXTURE_BUFFER, totalVoxels * sizeof(uint32_t), voxelCount.data(), GL_STATIC_DRAW);
}


//voxelVount->denseMap
void Instance::denseEstimationPass(float p) {
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_3D, texVoxelCount);
	//glBindImageTexture(0, texDenseMap, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, texDenseMap);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, debugUINT);

	denseEstimationShader.use();
	denseEstimationShader.setInt("totalSize", totalVoxels);
	denseEstimationShader.setInt("nVoxels_X", nVoxels_X);
	denseEstimationShader.setInt("nVoxels_Y", nVoxels_Y);
	denseEstimationShader.setInt("nVoxels_Z", nVoxels_Z);
	denseEstimationShader.setInt("kernelR", p*nVoxels_Z);
	denseEstimationShader.setFloat("voxelUnitSize", voxelUnitSize);
	glDispatchCompute(1+(unsigned int)totalVoxels/128, 1, 1);

	// make sure writing to buffer has finished before read
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	////explicitly unbind textures and images after use
	//glBindTexture(GL_TEXTURE_3D, 0);
	//glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	//glBindBuffer(GL_ARRAY_BUFFER, texDenseMap); 
	//float* bufferData = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
	//for (int i = 0; i < voxelCount.size(); i++) {
	//	if(i==920)
	//		std::cout<<bufferData[i];
	//}

	//glBindBuffer(GL_ARRAY_BUFFER, debugUINT);
	//uint32_t * debugData = (uint32_t*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
	//for (int i = 0; i < totalVoxels; i++) {
	//	if (debugData[i] != 0)
	//		std::cout << debugData[i];

	//}
}

//update tubes, stores to texUpdatedTubes
void Instance::advectionPass(float p) {
	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, texOriTracks);
	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, texUpdatedTracks);

	//set binding buffers for compute pass
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, texUpdatedTubes);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, debugFLOAT);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, texTempNormals);

	advectionShader.use();
	advectionShader.setInt("totalSize", 2*tubes.size());
	advectionShader.setInt("nVoxels_X", nVoxels_X);
	advectionShader.setInt("nVoxels_Y", nVoxels_Y);
	advectionShader.setInt("nVoxels_Z", nVoxels_Z);
	advectionShader.setInt("kernelR", p*nVoxels_Z);
	advectionShader.setFloat("voxelUnitSize", voxelUnitSize);
	advectionShader.setVec3("aabbMin", aabb.minPos);
	advectionShader.setInt("totalVoxels", totalVoxels);
	glDispatchCompute(1 + (unsigned int)2*tubes.size() / 128, 1, 1);

	// make sure writing to buffer has finished before read
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	//glBindBuffer(GL_ARRAY_BUFFER, debugFLOAT);
	//float * debugData = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
	//for (int i = 0;i<6 *tubes.size(); i++) {
	//	std::cout << debugData[i];
	//}
}

void Instance::smoothPass(float p) {
	int smoothL = p * nVoxels_Z;
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, texIsFiberEndpoint);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, texSmoothedTubes);

	smoothShader.use();
	smoothShader.setInt("totalSize", 2 * tubes.size());
	smoothShader.setFloat("smoothFactor",smoothFactor);
	smoothShader.setInt("smoothL", smoothL);
	smoothShader.setFloat("voxelUnitSize", voxelUnitSize);

	glDispatchCompute(1 + (unsigned int)2 * tubes.size() / 128, 1, 1);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Instance::relaxationPass() {

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texTempTubes);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, texRelaxedTubes);

	relaxShader.use();
	relaxShader.setInt("totalSize", 2 * tubes.size());
	relaxShader.setFloat("relaxFactor", relaxFactor);

	glDispatchCompute(1 + (unsigned int)2 * tubes.size() / 128, 1, 1);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Instance::updateDirectionPass() {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, texUpdatedDirections);

	updateDirectionShader.use();
	updateDirectionShader.setInt("totalSize", 2 * tubes.size());

	glDispatchCompute(1 + (unsigned int)2 * tubes.size() / 128, 1, 1);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Instance::updateNormalPass() {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, texUpdatedNormals);

	updateNormalShader.use();
	updateNormalShader.setInt("totalSize", 2 * tubes.size());

	glDispatchCompute(1 + (unsigned int)2 * tubes.size() / 128, 1, 1);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Instance::forceConsecutivePass() {
	forceConsecutiveShader.use();
	forceConsecutiveShader.setInt("totalSize", 2 * tubes.size());

	glDispatchCompute(1 + (unsigned int)2 * tubes.size() / 128, 1, 1);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Instance::transferDataGPU(GLuint srcBuffer, GLuint dstBuffer, size_t copySize) {
	glBindBuffer(GL_ARRAY_BUFFER,srcBuffer);
	glBindBuffer(GL_COPY_WRITE_BUFFER, dstBuffer);

	// Transfer data from buffer 1 to buffer 2
	glCopyBufferSubData(GL_ARRAY_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, copySize);

	// Unbind buffers
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

	//For synchronization
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void Instance::initVertexBufferLineMode() {
	if (-1 != VAOLines) {
		//destroy old buffers 
		glDeleteBuffers(1, &VBOLines);
	}
	glGenVertexArrays(1, &VAOLines);
	glGenBuffers(1, &VBOLines);
	glGenBuffers(1, &NBOLines);
	glGenBuffers(1, &DBOLines);

	glBindVertexArray(VAOLines);
	glBindBuffer(GL_ARRAY_BUFFER, VBOLines);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * tubes.size(), tubes.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, NBOLines);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * normals.size(), normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, DBOLines);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * directions.size(), directions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);
}

void Instance::initLineNormals() {
	for (int i = 0; i < tubes.size(); i++) {
		glm::vec3 vec;
		if (1==isFiberEndpoint[2*i+1])
			vec = glm::vec3(0, 0, 1);
		else
			vec = directions[2*(i + 1)];
		glm::vec3 left = glm::cross(vec, directions[2*i]);
		//glm::vec3 normal = glm::normalize(glm::cross(directions[i], left));
		glm::vec3 normal = glm::normalize(glm::cross(directions[2*i], left));
		normals.push_back(normal);
		normals.push_back(normal);
	}
}

void Instance::initLineDirections() {
	for (int i = 0; i < tubes.size(); i++) {
		glm::vec3 dir = glm::normalize(tubes[i].p1 - tubes[i].p2);
		directions.push_back(dir);
		directions.push_back(dir);
	}
}
