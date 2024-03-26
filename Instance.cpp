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
	std::shared_ptr<ComputeShader> _voxelCountShader,
	std::shared_ptr<ComputeShader> _denseEstimationShaderX,
	std::shared_ptr<ComputeShader> _denseEstimationShaderY,
	std::shared_ptr<ComputeShader> _denseEstimationShaderZ,
	std::shared_ptr<ComputeShader> _advectionShader,
	std::shared_ptr<ComputeShader> _smoothShader,
	std::shared_ptr<ComputeShader> _relaxShader,
	std::shared_ptr<ComputeShader> _updateDirectionShader,
	std::shared_ptr<ComputeShader> _slicingShader,
	std::shared_ptr<ComputeShader> _trackToLinesShader,
	std::shared_ptr<ComputeShader> _denseEstimationShader3D
)
	:voxelCountShader(_voxelCountShader),
	denseEstimationShader3D(_denseEstimationShader3D),
	denseEstimationShaderX(_denseEstimationShaderX),
	denseEstimationShaderY(_denseEstimationShaderY),
	denseEstimationShaderZ(_denseEstimationShaderZ),
	advectionShader(_advectionShader),
	smoothShader(_smoothShader),
	relaxShader(_relaxShader),
	updateDirectionShader(_updateDirectionShader),
	slicingShader(_slicingShader),
	trackToLinesShader(_trackToLinesShader)
{
	loadTracksFromTCK(path);
	spaceVoxelization();
	trackResampling();
	initAddition();
	updateTubes(tracks);
	//initLineDirections();
	//initLineNormals();

	//Triangle mode
	//updateTriangles(radius, nTris);
	//updateVertexIndiceBuffer();
	
	////Line mode
	//initVertexBufferLineMode();

	//initTextures();
	////initSSBOBinding();
	////initCudaMemory();
	//isActivated = true;
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
	int sample = 3;
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
	//std::vector<float>aveLength;
	//for (int i = 0; i < trackOffset.size(); i++) {
	//	float sum = 0;
	//	for (int j = trackOffset[i]; j < trackOffset[i] + trackSize[i]-1; j++) {
	//		float leng = glm::dot(tracks[j] - tracks[j + 1], tracks[j] - tracks[j + 1]);
	//		sum += sqrt(leng);
	//	}
	//	sum /= trackSize[i];
	//	aveLength.push_back(sum);
	//}
	
	float delta = nVoxels_Z * voxelUnitSize / 150;
	//float delta = voxelUnitSize;
	std::vector<glm::vec3>resampledTracks;
	std::vector<uint32_t>tempTrackOffset;
	std::vector<uint32_t>tempTrackSize;
	for (int i = 0; i < trackOffset.size(); i++) {
		tempTrackOffset.push_back(resampledTracks.size());
		for (int j = trackOffset[i]; j < trackOffset[i] + trackSize[i]-1; j++) {
			glm::vec3 dir = glm::normalize(tracks[j+1] - tracks[j]);
			float leng = sqrt(glm::dot(tracks[j] - tracks[j + 1], tracks[j] - tracks[j + 1]));
			int n = leng / delta;
			for (int k = 0; k < n; k++) {
				glm::vec3 newPoint = tracks[j]+k*delta*dir;
				resampledTracks.push_back(newPoint);
			}
		}
		resampledTracks.push_back(tracks[trackOffset[i] + trackSize[i] - 1]);
		tempTrackSize.push_back(resampledTracks.size() - tempTrackOffset[i]);
	}
	tracks = resampledTracks;
	trackOffset = tempTrackOffset;
	trackSize = tempTrackSize;
}

//initialize additional information: isFiberBegin, isFiberEnd, trackId
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
	lineBufferSize = 2 * tracks.size() - 2 * trackOffset.size();
}

////Recreate tubes from new tracks
////Input: current tracks
////Output: updated tubes
//void Instance::updateTubes(std::vector<glm::vec3>& currentTracks) {
//	tubes.clear();
//	int offset = 0;
//	for (int i = 0; i < trackOffset.size(); i++) {
//		for (int j = trackOffset[i]; j < trackOffset[i]+trackSize[i]-1; j++) {
//			/*tubes.push_back(Tube{ currentTracks[j] ,currentTracks[j + 1]});*/
//			tubes.push_back(Tube{ currentTracks.at(j) ,currentTracks.at(j+1) });
//			float leng = glm::dot(currentTracks[j] - currentTracks[j + 1], currentTracks[j] - currentTracks[j + 1]);
//			if (j == trackOffset[i]) {
//				isFiberEndpoint.push_back(1);
//			}
//			else {
//				isFiberEndpoint.push_back(0);
//				isFiberEndpoint.push_back(0);
//			}
//		}
//		isFiberEndpoint.push_back(1);
//	}
//}

//
////Output: updated vertices and indices
//void Instance::updateTriangles(float radius, int nTris) {
//	vertices.clear();
//	indices.clear();
//	float delta = 2 * glm::pi<float>() / nTris;
//	int offset = 0;
//	for (int i = 0; i < tubes.size(); i++) {
//		Tube tube = tubes[i];
//		glm::vec3 up = glm::vec3(0, 0, 1);
//		glm::vec3 ez = glm::normalize(tube.p2 - tube.p1);
//		glm::vec3 ey = cross(up, ez);
//		glm::vec3 ex = cross(ez, ey);
//
//		glm::mat3 R = glm::mat3(ex, ey, ez);
//		glm::vec3 T = tube.p1;
//		float leng = sqrt(glm::dot(tubes[i].p2 - tubes[i].p1, tubes[i].p2 - tubes[i].p1));
//		for (int k = 0; k < nTris; k++) {
//			float theta = delta * k;
//			vertices.push_back(Vertex{ glm::vec4(R*glm::vec3(radius*glm::cos(theta),radius*glm::sin(theta),leng) + T,0) });
//		}
//		for (int k = 0; k < nTris; k++) {
//			float theta = delta * k;
//			vertices.push_back(Vertex{ glm::vec4(R*glm::vec3(radius*glm::cos(theta),radius*glm::sin(theta),0) + T,0) });
//		}
//		for (int k = 0; k < nTris; k++) {
//			indices.push_back(offset + k);
//			indices.push_back(offset + nTris + k);
//			indices.push_back(offset + (k + 1) % nTris);
//
//			indices.push_back(offset + (k + 1) % nTris);
//			indices.push_back(offset + nTris + k);
//			indices.push_back(offset + nTris + (k + 1) % nTris);
//		}
//		offset += 2 * nTris;
//	}
//
//	////debug
//	//vertices = { Vertex{glm::vec3(-0.5f, -0.5f, -10)},Vertex{glm::vec3(0.5f, -0.5f, -10)},Vertex{glm::vec3(0.5f, 0.5f, -10)} };
//	//indices = { 0,1,2 };
//}

void Instance::setVisible(bool _visible) {
	visible = _visible;
}

//void Instance::updateVertexIndiceBuffer() {
//	if (-1!=VAO) {
//	    //destroy old buffers 
//		glDeleteBuffers(1, &VBO);
//		glDeleteBuffers(1, &IBO);
//	}
//	glGenVertexArrays(1, &VAO);
//	glGenBuffers(1, &VBO);
//	glGenBuffers(1, &IBO);
//
//	glBindVertexArray(VAO);
//
//	//VBO stores vertex positions, possibly extended to store more attributes
//	glBindBuffer(GL_ARRAY_BUFFER, VBO);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
//	glEnableVertexAttribArray(0);
//
//	//IBO stores indices
//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
//	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);
//
//	glBindVertexArray(0);
//}

void Instance::drawLineMode(float lineWidth) {
	glBindVertexArray(VAOLines);
	glLineWidth(lineWidth);
	glDrawArrays(GL_LINES, 0, lineBufferSize);

	glBindVertexArray(0);
}

//void Instance::recreateMeshNewRadius(float radius, int nTris) {
//	updateTriangles(radius, nTris);
//	//load new data to gpu
//	glBindVertexArray(VAO);
//	glBindBuffer(GL_ARRAY_BUFFER, VBO);
//	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * vertices.size(), vertices.data());
//
//	glBindBuffer(GL_ARRAY_BUFFER, 0);
//	glBindVertexArray(0);
//}
//
//void Instance::recreateMeshNewNTris(float radius, int nTris) {
//	//create meshes by new parameters
//	updateTriangles(radius, nTris);
//	//recreate VBO and IBO, load new data
//	updateVertexIndiceBuffer();
//}

//void Instance::edgeBundling(float p, float radius, int nTris){
//	resampling();
//	std::vector<float>denseMap = computeDensity(p);
//	std::vector<glm::vec3> newTracks = advection(denseMap,p);
//	std::vector<glm::vec3> smoothTracks = smoothing(newTracks);
//	updateTubes(smoothTracks);
//	updateTriangles(radius,nTris);
//	updateVertexIndiceBuffer();
//}

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

////Input: voxelAssignment, parameter Pr
////Output: 3D dense map of size nVertices
//std::vector<float> Instance::computeDensity(float p) {
//	std::vector<float>denseMap;
//
//	denseMap.resize(nVoxels_X*nVoxels_Y*nVoxels_Z);
//	int kernelR = p * nVoxels_Z;
//	for (int i = 0; i < denseMap.size(); i++) {
//		if(i%1000==0)
//		std::cout << i << std::endl;
//		float dense = 0;
//		int Z = i / (nVoxels_X*nVoxels_Y);
//		int mod= i % (nVoxels_X*nVoxels_Y);
//		int X = mod % nVoxels_X;
//		int Y = mod / nVoxels_X;
//		for (int dx = -kernelR; dx < kernelR; dx++) {
//			for (int dy = -kernelR; dy < kernelR; dy++) {
//				for (int dz = -kernelR; dz < kernelR; dz++) {
//					int nx = X + dx;
//					nx = std::max(nx, 0);
//					nx = std::min(nx, int(nVoxels_X - 1));
//					int ny = Y + dy;
//					ny = std::max(ny, 0);
//					ny = std::min(ny, int(nVoxels_Y - 1));
//					int nz = Z + dz;
//					nz = std::max(nz, 0);
//					nz = std::min(nz, int(nVoxels_Z - 1));
//					glm::vec3 diff = voxelUnitSize * (glm::vec3(nx, ny, nz) - glm::vec3(X, Y, Z));
//					float dot = glm::dot(diff, diff);
//					float PR2 = kernelR * voxelUnitSize*kernelR * voxelUnitSize;
//					if (dot > PR2)
//						continue;
//
//					uint32_t index = nVoxels_X * nVoxels_Y*nz + nVoxels_X * ny + nx;
//					index = std::min(int(index), int(voxelCount.size()));
//					int pointCnt = voxelCount[index];
//					
//					dense += pointCnt * (1 - dot/PR2);
//				}
//			}
//		}
//		denseMap[i] = dense;
//		if (i == 920)
//			std::cout << dense;
//	}
//
//	return denseMap;
//}

////Input: dense map
////Output: updated tracks
//std::vector<glm::vec3> Instance::advection(std::vector<float>& denseMap, float p) {
//	std::vector<glm::vec3> newTracks;
//	int kernelR = p * nVoxels_Z;
//	int R2 = kernelR * kernelR;
//	int kernelWidth = 2 * kernelR + 1;
//
//	for (int i = 0; i < tracks.size(); i++) {
//		if (i % 5 == 0)
//			std::cout << i << std::endl;
//		glm::vec3 grad = glm::vec3(0);
//		glm::vec3 point = tracks.at(i);
//		glm::vec3 deltaP = point - aabb.minPos;
//		int X = deltaP.x / voxelUnitSize;
//		int Y = deltaP.y / voxelUnitSize;
//		int Z = deltaP.z / voxelUnitSize;
//		X = std::min(X, int(nVoxels_X - 1));
//		Y = std::min(Y, int(nVoxels_Y - 1));
//		Z = std::min(Z, int(nVoxels_Z - 1));
//		uint32_t indexA= nVoxels_X * nVoxels_Y*Z + nVoxels_X * Y + X;
//		for (int dx = -kernelR; dx < kernelR; dx++) {
//			for (int dy = -kernelR; dy < kernelR; dy++) {
//				for (int dz = -kernelR; dz < kernelR; dz++) {
//					int nx = X + dx;
//					nx = std::max(nx, 0);
//					nx = std::min(nx, int(nVoxels_X - 1));
//					int ny = Y + dy;
//					ny = std::max(ny, 0);
//					ny = std::min(ny, int(nVoxels_Y - 1));
//					int nz = Z + dz;
//					nz = std::max(nz, 0);
//					nz = std::min(nz, int(nVoxels_Z - 1));
//
//					glm::vec3 dir = glm::vec3(dx, dy, dz);
//					float diffPos = glm::dot(dir, dir) / R2;
//					if (diffPos < 1e-5)
//						continue;
//					if (diffPos > 1)
//						continue;
//					uint32_t indexB = nVoxels_X * nVoxels_Y*nz + nVoxels_X * ny + nx;
//					indexB = std::min(int(indexB), int(denseMap.size()));
//					float diffDense = denseMap[indexB] - denseMap[indexA];
//
//					glm::vec3 old = grad;
//					grad += glm::normalize(dir)*diffDense*expf(-diffPos);
//		
//					//debug
//					if (std::isnan(grad.x)&& !std::isnan(old.x)) {
//						std::cout << glm::normalize(dir).x <<" "<<diffDense << " "<< expf(-diffPos)<< std::endl;
//					}
//				}
//			}
//		}
//		grad = grad/float(kernelWidth * kernelWidth*kernelWidth);
//		glm::vec3 newPoint;
//		if (glm::dot(grad, grad) < 1e-5)
//			newPoint = tracks[i];
//		else
//		    newPoint = tracks[i] + kernelR*voxelUnitSize * glm::normalize(grad);
//		newTracks.push_back(newPoint);
//	}
//	return newTracks;
//}

////Input: tracks
////Output: updated tracks
//std::vector<glm::vec3> Instance::smoothing(std::vector<glm::vec3>& newTracks) {
//	return newTracks;
//}

void Instance::edgeBundlingGPU(float _p) {
	bundle = _p;
	//repeat bundling for several iterations

	//TESTING
	//init texTempTracks with original tracks
	glBindBuffer(GL_TEXTURE_BUFFER, texTempTracks);
	glBufferData(GL_TEXTURE_BUFFER, tracks.size() * 3 * sizeof(float), tracks.data(), GL_STATIC_DRAW);

	float p = _p;
	for (int i = 0; i < nIters; i++) {
		GLuint clearValue = 0;
		//clear voxelCount at the beginning of each iteration
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, texVoxelCount);
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &clearValue);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		voxelCountPass();
		denseEstimationPass(p);
		advectionPass(p);
		smoothPass(p);
		//re-transfer original tracks to texTempTubes, for relaxation pass
		glBindBuffer(GL_TEXTURE_BUFFER, texTempTracks);
		glBufferData(GL_TEXTURE_BUFFER, tracks.size() * 3 * sizeof(float), tracks.data(), GL_STATIC_DRAW);
		relaxationPass();

		transferDataGPU(texRelaxedTracks, texTempTracks, tracks.size() * 3 * sizeof(float));  //copy data from resulting updated to temp for next iteration
		p *= 0.95;
	}
	trackToLinePass(texRelaxedTracks,texRelaxedLines);   //map from tracks to lines for rendering
	transferDataGPU(texRelaxedLines, VBOLines, size_t(lineBufferSize) * 3 * sizeof(float));
	//trackToLinePass(texRelaxedTracks, texRelaxedLines);
	//trackToLinePass(texRelaxedTracks, VBOLines);  

	//update slicing
	if (enableSlicling) {
		slicing(slicingPos,slicingDir);
	}

	//triangle mode
	//updateTubes(newTracks);
	//updateTriangles(radius, nTris);
	//updateVertexIndiceBuffer();
}

//tubes->voxelCount
void Instance::voxelCountPass() {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texTempTracks);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, texVoxelCount);

	voxelCountShader->use();
	voxelCountShader->setInt("totalSize", tracks.size());
	voxelCountShader->setInt("nVoxels_X", nVoxels_X);
	voxelCountShader->setInt("nVoxels_Y", nVoxels_Y);
	voxelCountShader->setInt("nVoxels_Z", nVoxels_Z);
	voxelCountShader->setVec3("aabbMin", aabb.minPos);
	voxelCountShader->setFloat("voxelUnitSize", voxelUnitSize);
	voxelCountShader->setInt("totalVoxels", totalVoxels);

	glDispatchCompute(1 + (unsigned int)tracks.size() / 128, 1, 1);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}


//voxelVount->denseMap
void Instance::denseEstimationPass(float p) {
	//auto start = std::chrono::high_resolution_clock::now();
	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texVoxelCount);
	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, texDenseMapTest);
	//denseEstimationShader3D->use();
	//denseEstimationShader3D->setInt("totalSize", totalVoxels);
	//denseEstimationShader3D->setInt("nVoxels_X", nVoxels_X);
	//denseEstimationShader3D->setInt("nVoxels_Y", nVoxels_Y);
	//denseEstimationShader3D->setInt("nVoxels_Z", nVoxels_Z);
	//denseEstimationShader3D->setInt("kernelR", p*nVoxels_Z);
	//denseEstimationShader3D->setFloat("voxelUnitSize", voxelUnitSize);
	//glDispatchCompute(1+(unsigned int)totalVoxels/128, 1, 1);

	//// make sure writing to buffer has finished before read
	//glMemoryBarrier(GL_ALL_BARRIER_BITS);
	//auto end = std::chrono::high_resolution_clock::now();
	//auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	////std::cout << "Elapsed time: " << duration.count() << " microseconds" << std::endl;

	auto start = std::chrono::high_resolution_clock::now();
	//Convolution on X
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texVoxelCount);
	glBindImageTexture(0, texDenseX, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

	denseEstimationShaderX->use();
	denseEstimationShaderX->setInt("totalSize", totalVoxels);
	denseEstimationShaderX->setInt("nVoxels_X", nVoxels_X);
	denseEstimationShaderX->setInt("nVoxels_Y", nVoxels_Y);
	denseEstimationShaderX->setInt("nVoxels_Z", nVoxels_Z);
	denseEstimationShaderX->setInt("kernelR", p * nVoxels_Z);
	denseEstimationShaderX->setFloat("voxelUnitSize", voxelUnitSize);
	glDispatchCompute(1 + (unsigned int)totalVoxels / 128, 1, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	//Convolution on Y
	glBindImageTexture(0, texDenseX, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
	glBindImageTexture(1, texDenseY, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

	denseEstimationShaderY->use();
	denseEstimationShaderY->setInt("totalSize", totalVoxels);
	denseEstimationShaderY->setInt("nVoxels_X", nVoxels_X);
	denseEstimationShaderY->setInt("nVoxels_Y", nVoxels_Y);
	denseEstimationShaderY->setInt("nVoxels_Z", nVoxels_Z);
	denseEstimationShaderY->setInt("kernelR", p * nVoxels_Z);
	denseEstimationShaderY->setFloat("voxelUnitSize", voxelUnitSize);
	glDispatchCompute(1 + (unsigned int)totalVoxels / 128, 1, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	//Convolution on Z
	glBindImageTexture(0, texDenseY, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
	glBindImageTexture(1, texDenseMap, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

	denseEstimationShaderZ->use();
	denseEstimationShaderZ->setInt("totalSize", totalVoxels);
	denseEstimationShaderZ->setInt("nVoxels_X", nVoxels_X);
	denseEstimationShaderZ->setInt("nVoxels_Y", nVoxels_Y);
	denseEstimationShaderZ->setInt("nVoxels_Z", nVoxels_Z);
	denseEstimationShaderZ->setInt("kernelR", p * nVoxels_Z);
	denseEstimationShaderZ->setFloat("voxelUnitSize", voxelUnitSize);
	glDispatchCompute(1 + (unsigned int)totalVoxels / 128, 1, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	//std::cout << "Elapsed time: " << duration.count() << " microseconds" << std::endl;
}

//update tubes, stores to texUpdatedTubes
void Instance::advectionPass(float p) {
	glBindImageTexture(0, texDenseMap, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texTempTracks);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, texUpdatedTracks);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, texTempNormals);

	advectionShader->use();
	advectionShader->setInt("totalSize", tracks.size());
	advectionShader->setInt("nVoxels_X", nVoxels_X);
	advectionShader->setInt("nVoxels_Y", nVoxels_Y);
	advectionShader->setInt("nVoxels_Z", nVoxels_Z);
	advectionShader->setInt("kernelR", p*nVoxels_Z);
	advectionShader->setFloat("voxelUnitSize", voxelUnitSize);
	advectionShader->setVec3("aabbMin", aabb.minPos);
	advectionShader->setInt("totalVoxels", totalVoxels);
	glDispatchCompute(1 + (unsigned int)tracks.size() / 128, 1, 1);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	//glBindBuffer(GL_ARRAY_BUFFER, debugFLOAT);
	//float * debugData = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
	//for (int i = 0;i<6 *tubes.size(); i++) {
	//	std::cout << debugData[i];
	//}
}

void Instance::smoothPass(float p) {
	int smoothL = p * nVoxels_Z;

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texUpdatedTracks);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, texSmoothedTracks);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, texIsFiberBegin);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, texIsFiberEnd);

	smoothShader->use();
	smoothShader->setInt("totalSize", tracks.size());
	smoothShader->setFloat("smoothFactor",smoothFactor);
	smoothShader->setInt("smoothL", smoothL);
	smoothShader->setFloat("voxelUnitSize", voxelUnitSize);

	glDispatchCompute(1 + (unsigned int)tracks.size() / 128, 1, 1);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void Instance::relaxationPass() {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texTempTracks);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, texSmoothedTracks);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, texRelaxedTracks);

	relaxShader->use();
	relaxShader->setInt("totalSize", tracks.size());
	relaxShader->setFloat("relaxFactor", relaxFactor);

	glDispatchCompute(1 + (unsigned int)tracks.size() / 128, 1, 1);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void Instance::updateDirectionPass() {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texRelaxedTracks);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, texUpdatedDirections);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, texIsFiberBegin);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, texIsFiberEnd);

	updateDirectionShader->use();
	updateDirectionShader->setInt("totalSize", tracks.size());

	glDispatchCompute(1 + (unsigned int)tracks.size() / 128, 1, 1);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
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

int Instance::getNVoxelsX() {
	return nVoxels_X;
}

int Instance::getNVoxelsY() {
	return nVoxels_Y;
}

int Instance::getNVoxelsZ() {
	return nVoxels_Z;
}

glm::vec3 Instance::getAABBMin() {
	return aabb.minPos;
}

float Instance::getVoxelUnitSize() {
	return voxelUnitSize;
}

int Instance::getTotalVoxels() {
	return totalVoxels;
}

GLuint Instance::getDenseMap() {
	return texDenseMap;
}

GLuint Instance::getVoxelCount() {
	return texVoxelCount;
}

void Instance::slicing(glm::vec3 pos, glm::vec3 dir) {
	if (!enableSlicling)
		return;
	if (glm::dot(dir, dir) < 1e-3)
		return;
	if (glm::dot(pos, pos) < 1e-2)
		return;

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texRelaxedLines);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, VBOLines);

	slicingPos = pos;
	slicingDir = dir;

	glm::vec3 _slicingPos = aabb.minPos+(aabb.maxPos-aabb.minPos)*pos;
	glm::vec3 _slicingDir = dir;

	slicingShader->use();
	slicingShader->setInt("totalSize", lineBufferSize/2);
	slicingShader->setVec3("pos", _slicingPos);
	slicingShader->setVec3("dir", _slicingDir);

	glDispatchCompute(1 + (unsigned int)(lineBufferSize / 2) / 128, 1, 1);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void Instance::updateEnableSlicing(glm::vec3 pos, glm::vec3 dir) {
	enableSlicling = !enableSlicling;
	slicingPos = pos;
	slicingDir = dir;
	if (enableSlicling)
		slicing(slicingPos, slicingDir);
	else
		transferDataGPU(texRelaxedLines, VBOLines, lineBufferSize*3*sizeof(float));
		//trackToLinePass(texRelaxedTracks, VBOLines);
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
	edgeBundlingGPU(bundle);
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
    texTempNormals,
    texUpdatedNormals,
    texTempDirections,
    texUpdatedDirections,
    texVoxelCount,
    texIsFiberBegin,
	texIsFiberEnd,
	texTrackId,
	texRelaxedLines
	};
	glDeleteBuffers(15, buffersToDelete);

	glDeleteVertexArrays(1,&VAOLines);

	GLuint texturesToDelete[] = { 
		texDenseX,
		texDenseY,
		texDenseMap,
	};
	glDeleteTextures(3, texturesToDelete);
}

void Instance::createTextures() {
	glGenVertexArrays(1, &VAOLines);
	glGenBuffers(1, &VBOLines);
	glGenBuffers(1, &DBOLines);

	glBindVertexArray(VAOLines);
	glBindBuffer(GL_ARRAY_BUFFER, VBOLines);
	glBufferData(GL_ARRAY_BUFFER, size_t(lineBufferSize) * 3 * sizeof(float), NULL, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, DBOLines);
	glBufferData(GL_ARRAY_BUFFER, size_t(lineBufferSize) * 3 * sizeof(float), NULL, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	//temp tracks
	glGenBuffers(1, &texTempTracks);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texTempTracks);
	glBufferData(GL_SHADER_STORAGE_BUFFER, tracks.size() * 3 * sizeof(float), NULL, GL_STATIC_DRAW);

	//updated tracks
	glGenBuffers(1, &texUpdatedTracks);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texUpdatedTracks);
	glBufferData(GL_SHADER_STORAGE_BUFFER, tracks.size() * 3 * sizeof(float), NULL, GL_STATIC_DRAW);

	//smoothed tracks
	glGenBuffers(1, &texSmoothedTracks);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texSmoothedTracks);
	glBufferData(GL_SHADER_STORAGE_BUFFER, tracks.size() * 3 * sizeof(float), NULL, GL_STATIC_DRAW);

	//relaxed tracks
	glGenBuffers(1, &texRelaxedTracks);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texRelaxedTracks);
	glBufferData(GL_SHADER_STORAGE_BUFFER, tracks.size() * 3 * sizeof(float), NULL, GL_STATIC_DRAW);

	//temp normals
	glGenBuffers(1, &texTempNormals);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texTempNormals);
	glBufferData(GL_SHADER_STORAGE_BUFFER, tracks.size(), NULL, GL_STATIC_DRAW);

	//updated normals
	glGenBuffers(1, &texUpdatedNormals);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texUpdatedNormals);
	glBufferData(GL_SHADER_STORAGE_BUFFER, tracks.size(), NULL, GL_STATIC_DRAW);

	//temp directions
	glGenBuffers(1, &texTempDirections);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texTempDirections);
	glBufferData(GL_SHADER_STORAGE_BUFFER, tracks.size(), NULL, GL_STATIC_DRAW);

	//updated directions
	glGenBuffers(1, &texUpdatedDirections);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texUpdatedDirections);
	glBufferData(GL_SHADER_STORAGE_BUFFER, tracks.size(), NULL, GL_STATIC_DRAW);

	//voxelCount (1-D buffer)
	glGenBuffers(1, &texVoxelCount);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texVoxelCount);
	glBufferData(GL_SHADER_STORAGE_BUFFER, totalVoxels * sizeof(uint32_t), NULL, GL_STATIC_DRAW);

	//denseX (3D texture)
	glGenTextures(1, &texDenseX);
	glBindTexture(GL_TEXTURE_3D, texDenseX);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, nVoxels_X, nVoxels_Y, nVoxels_Z, 0, GL_RED, GL_FLOAT, NULL);

	//denseY (3D texture)
	glGenTextures(1, &texDenseY);
	glBindTexture(GL_TEXTURE_3D, texDenseY);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, nVoxels_X, nVoxels_Y, nVoxels_Z, 0, GL_RED, GL_FLOAT, NULL);

	//denseMap (3D texture)
	glGenTextures(1, &texDenseMap);
	glBindTexture(GL_TEXTURE_3D, texDenseMap);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, nVoxels_X, nVoxels_Y, nVoxels_Z, 0, GL_RED, GL_FLOAT, NULL);

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

	//relaxedLines
	glGenBuffers(1, &texRelaxedLines);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texRelaxedLines);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size_t(lineBufferSize) * 3 * sizeof(float), NULL, GL_STATIC_DRAW);

	//init VBO by original tracks (using a compute shader)
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texTempTracks);
	glBufferData(GL_SHADER_STORAGE_BUFFER, tracks.size() * 3 * sizeof(float), tracks.data(), GL_STATIC_DRAW);
	trackToLinePass(texTempTracks,VBOLines);

	//init texRelaxedlines for slicing
	transferDataGPU(VBOLines, texRelaxedLines, size_t(lineBufferSize) * 3 * sizeof(float));

	//init DBO (using a compute shader)
	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, texRelaxedTracks);
	//glBufferData(GL_SHADER_STORAGE_BUFFER, tracks.size() * 3 * sizeof(float), tracks.data(), GL_STATIC_DRAW);
	//updateDirectionPass();
	//trackToLinePass(texUpdatedDirections, DBOLines);

	//DEBUG
	glBindBuffer(GL_ARRAY_BUFFER, DBOLines);
	glBufferData(GL_ARRAY_BUFFER, size_t(lineBufferSize) * 3 * sizeof(float), directions.data(), GL_STATIC_DRAW);
}

//DEBUG
void Instance::updateTubes(std::vector<glm::vec3>& currentTracks) {
	tubes.clear();
	int offset = 0;
	for (int i = 0; i < trackOffset.size(); i++) {
		for (int j = trackOffset[i]; j < trackOffset[i] + trackSize[i] - 1; j++) {
			/*tubes.push_back(Tube{ currentTracks[j] ,currentTracks[j + 1]});*/
			tubes.push_back(Tube{ currentTracks.at(j) ,currentTracks.at(j + 1) });
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