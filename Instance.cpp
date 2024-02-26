#include"Instance.h"
#include<fstream>
#include"glm/glm.hpp"
#include <glm/gtc/constants.hpp>
#include<iostream>
#include <algorithm>

#define INFINITE 999999999

Instance::Instance(std::string path, float radius, int nTris) {
	loadTracksFromTCK(path);
	updateTubes(tracks);
	updateTriangles(radius, nTris);
	updateVertexIndiceBuffer();
	spaceVoxelization();
	////debug
	//vertices = { Vertex{glm::vec3(0.5f,  0.5f, 0.0f)},Vertex{glm::vec3(0.5f, -0.5f, 0.0f)},Vertex{glm::vec3(-0.5f, -0.5f, 0.0f)},Vertex{glm::vec3(-0.5f,  0.5f, 0.0f)} };
	//indices = { 0,1,3,1,2,3 };
	//initVertexIndiceBuffer();
}

Instance::~Instance() {

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

std::vector<glm::vec3> Instance::readTCK(const std::string& filename, int offset) {
	std::ifstream file(filename, std::ios::binary);
	// Read track data from file
	if (!file.is_open()) {
		std::runtime_error("failed to opening file!");
	}

	// Seek to the specified offset
	file.seekg(offset, std::ios::beg);

	std::vector<glm::vec3> nowTracks;
	//std::vector<glm::vec3> currentStreamline;
	int count = 0;
	int debug = 0;
	trackOffset.push_back(0);
	while (!file.eof()) {
		float x, y, z;
		file.read(reinterpret_cast<char*>(&x), sizeof(float));
		file.read(reinterpret_cast<char*>(&y), sizeof(float));
		file.read(reinterpret_cast<char*>(&z), sizeof(float));

		// Check if x is NaN, indicating the start of a new streamline
		if (std::isnan(x)) {
			if (debug % 50 != 0) {
				debug++;
				continue;
			}
			if (0!=count) {
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
			if (debug % 50 != 0) {
				continue;
			}
			count++;
			nowTracks.push_back(glm::vec3(x, y, z));
		}
	}
	trackOffset.pop_back();
	file.close();
	return nowTracks;
}

//Input: file
//Output: tracks
void Instance::loadTracksFromTCK(std::string path) {
	//load data from tck
	//std::string filename = "C:\\Users\\zzc_c\\Downloads\\whole_brain.tck"; 
	int OFFSET = 107;
	int count = 144000;
	tracks = readTCK(path, OFFSET);
}

//Recreate tubes from new tracks
//Input: current tracks
//Output: updated tubes
void Instance::updateTubes(std::vector<glm::vec3>& currentTracks) {
	tubes.clear();
	int offset = 0;
	for (int i = 0; i < trackOffset.size(); i++) {
		for (int j = trackOffset[i]; j < trackOffset[i]+trackSize[i]-1; j++) {
			tubes.push_back(Tube{ currentTracks[j] ,currentTracks[j + 1], });
		}
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

void Instance::draw() {
	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
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
	voxelCount.resize(nVoxels_X*nVoxels_Y*nVoxels_Z);
	std::fill(voxelCount.begin(), voxelCount.end(), 0);
	for (int i = 0; i < tracks.size(); i++) {
		glm::vec3 &point = tracks[i];
		glm::vec3 deltaP = point - aabb.minPos;
		uint32_t level_X = std::min(float(nVoxels_X - 1), deltaP.x / voxelUnitSize);
		uint32_t level_Y = std::min(float(nVoxels_Y - 1), deltaP.y / voxelUnitSize);
		uint32_t level_Z = std::min(float(nVoxels_Z - 1), deltaP.z / voxelUnitSize);
		uint32_t index = nVoxels_X * nVoxels_Y*level_Z + nVoxels_Y * level_X + level_Y;
		voxelCount[index]++;
	}
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
		float dense = 0;
		int Z = i / (nVoxels_X*nVoxels_Y);
		int mod= i % (nVoxels_X*nVoxels_Y);
		int X = mod / nVoxels_Y;
		int Y = mod % nVoxels_Y;
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

					uint32_t index = nVoxels_X * nVoxels_Y*nz + nVoxels_Y * nx + ny;
					index = std::min(int(index), int(voxelCount.size()));
					int pointCnt = voxelCount[index];
					
					dense += pointCnt * (1 - dot/PR2);
				}
			}
		}
		denseMap[i] = dense;
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
		glm::vec3 grad = glm::vec3(0);
		glm::vec3 point = tracks.at(i);
		glm::vec3 deltaP = point - aabb.minPos;
		int X = deltaP.x / voxelUnitSize;
		int Y = deltaP.y / voxelUnitSize;
		int Z = deltaP.z / voxelUnitSize;
		X = std::min(X, int(nVoxels_X - 1));
		Y = std::min(Y, int(nVoxels_Y - 1));
		Z = std::min(Z, int(nVoxels_Z - 1));
		uint32_t indexA= nVoxels_X * nVoxels_Y*Z + nVoxels_Y * X + Y;
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
					uint32_t indexB = nVoxels_X * nVoxels_Y*nz + nVoxels_Y * nx + ny;
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
	std::cout << "done" << std::endl;
	return newTracks;
}

//Input: tracks
//Output: updated tracks
std::vector<glm::vec3> Instance::smoothing(std::vector<glm::vec3>& newTracks) {
	return newTracks;
}
