#include"FiberBundler.h"
#include <chrono>

#define INFINITE 999999999

FiberBundler::FiberBundler() {
}

FiberBundler::~FiberBundler() {
	
}

void FiberBundler::init() {
	initComputeShaders();
}

void FiberBundler::initComputeShaders() {
	denseEstimationShaderX = new ComputeShader("shaders/denseEstimationX.cs");
	denseEstimationShaderY = new ComputeShader("shaders/denseEstimationY.cs");
	denseEstimationShaderZ = new ComputeShader("shaders/denseEstimationZ.cs");
	advectionShader = new ComputeShader("shaders/advection.cs");
	voxelCountShader = new ComputeShader("shaders/voxelCount.cs");
	smoothShader = new ComputeShader("shaders/smooth.cs");
	relaxShader = new ComputeShader("shaders/relaxation.cs");
	updateDirectionShader = new ComputeShader("shaders/updateDirections.cs");
	slicingShader = new ComputeShader("shaders/slicing.cs");
	trackToLinesShader = new ComputeShader("shaders/trackToLines.cs");
	denseEstimationShader3D = new ComputeShader("shaders/denseEstimation.cs");
}

void FiberBundler::initForInstance() {
	spaceVoxelization();
	trackResampling();
	initTextures();
	isEnable = true;
}

void FiberBundler::enable(Instance* _instance) {
	instance = _instance;
	initForInstance();

	//transfer the resampled fibers to VBO
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texTempTracks);
	glBufferData(GL_SHADER_STORAGE_BUFFER, resampledTracks.size() * 3 * sizeof(float), resampledTracks.data(), GL_STATIC_DRAW);
	trackToLinePass(texTempTracks, texRelaxedLines);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texTempDirections);
	glBufferData(GL_SHADER_STORAGE_BUFFER, resampledDirections.size() * 3 * sizeof(float), resampledDirections.data(), GL_STATIC_DRAW);
	trackToLinePass(texTempDirections, texUpdatedDirectionsLines);

	updateInstanceFibers();

	//perform initial bundling for the currently activated instance, using the parameter settings recorded in that instance
	float p = instance->getBundleValue();
	bundle = p;
	if (p > 0) {
		edgeBundlingGPU(p);
	}
}

void FiberBundler::disable() {
	destroyTextures();
	cleanHostMemory();
	isEnable = false;
	instance = nullptr;
	bundle = 0;
}

//Assign points to voxels
//Input: tracks
//Output: voxelAssignment
void FiberBundler::spaceVoxelization() {
	const std::vector<glm::vec3>& tracks = instance->getTracks();
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
	glm::vec3 delta = aabb.maxPos - aabb.minPos;
	voxelUnitSize = delta.z / nVoxels_Z;
	nVoxels_X = (uint32_t)(delta.x / voxelUnitSize) + 1;
	nVoxels_Y = (uint32_t)(delta.y / voxelUnitSize) + 1;

	totalVoxels = nVoxels_X * nVoxels_Y * nVoxels_Z;
}

//resample tracks to produce points with equal interval
void FiberBundler::trackResampling() {
	const std::vector<glm::vec3>& tracks = instance->getTracks();
	const std::vector<uint32_t>& trackOffset = instance->getTrackOffset();
	const std::vector<uint32_t>& trackSize = instance->getTrackSize();

	float delta = nVoxels_Z * voxelUnitSize / resampleFactor;
	for (int i = 0; i < trackOffset.size(); i++) {
		resampledTrackOffset.push_back(resampledTracks.size());
		for (int j = trackOffset[i]; j < trackOffset[i] + trackSize[i] - 1; j++) {
			glm::vec3 dir = glm::normalize(tracks[j + 1] - tracks[j]);
			float leng = sqrt(glm::dot(tracks[j] - tracks[j + 1], tracks[j] - tracks[j + 1]));
			int n = leng / delta;
			for (int k = 0; k < n; k++) {
				glm::vec3 newPoint = tracks[j] + k * delta * dir;
				resampledTracks.push_back(newPoint);
			}
		}
		resampledTracks.push_back(tracks[trackOffset[i] + trackSize[i] - 1]);
		resampledTrackSize.push_back(resampledTracks.size() - resampledTrackOffset[i]);
	}

	isFiberBegin.resize(resampledTracks.size());
	isFiberEnd.resize(resampledTracks.size());
	trackId.resize(resampledTracks.size());
	for (int i = 0; i < resampledTrackOffset.size(); i++) {
		isFiberBegin[resampledTrackOffset[i]] = 1;
		isFiberEnd[resampledTrackOffset[i] + resampledTrackSize[i] - 1] = 1;
	}
	for (int i = 0; i < resampledTrackOffset.size(); i++) {
		for (int k = 0; k < resampledTrackSize[i]; k++) {
			trackId[resampledTrackOffset[i] + k] = i;
		}
	}
	lineBufferSize = 2 * resampledTracks.size() - 2 * resampledTrackOffset.size();

	resampledDirections.resize(resampledTracks.size());
	for (int i = 0; i < resampledTracks.size(); i++) {
		glm::vec3 p1, p2;
		if (1 == isFiberBegin[i]) {
			p1 = resampledTracks[i];
			p2 = resampledTracks[i+1];
		}
		else if (1 == isFiberEnd[i]) {
			p1 = resampledTracks[i-1];
			p2 = resampledTracks[i];
		}
		else {
			p1 = resampledTracks[i];
			p2 = resampledTracks[i+1];
		}
		resampledDirections[i] = glm::normalize(p1 - p2);
	}
}

void FiberBundler::initTextures() {
	//temp tracks
	glGenBuffers(1, &texTempTracks);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texTempTracks);
	glBufferData(GL_SHADER_STORAGE_BUFFER, resampledTracks.size() * 3 * sizeof(float), NULL, GL_STATIC_DRAW);

	//updated tracks
	glGenBuffers(1, &texUpdatedTracks);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texUpdatedTracks);
	glBufferData(GL_SHADER_STORAGE_BUFFER, resampledTracks.size() * 3 * sizeof(float), NULL, GL_STATIC_DRAW);

	//smoothed tracks
	glGenBuffers(1, &texSmoothedTracks);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texSmoothedTracks);
	glBufferData(GL_SHADER_STORAGE_BUFFER, resampledTracks.size() * 3 * sizeof(float), NULL, GL_STATIC_DRAW);

	//relaxed tracks
	glGenBuffers(1, &texRelaxedTracks);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texRelaxedTracks);
	glBufferData(GL_SHADER_STORAGE_BUFFER, resampledTracks.size() * 3 * sizeof(float), NULL, GL_STATIC_DRAW);

	//temp directions
	glGenBuffers(1, &texTempDirections);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texTempDirections);
	glBufferData(GL_SHADER_STORAGE_BUFFER, resampledTracks.size() * 3 * sizeof(float), NULL, GL_STATIC_DRAW);

	//updated directions
	glGenBuffers(1, &texUpdatedDirections);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texUpdatedDirections);
	glBufferData(GL_SHADER_STORAGE_BUFFER, resampledTracks.size() * 3 * sizeof(float), NULL, GL_STATIC_DRAW);

	//updated direcions (Lines)
	glGenBuffers(1, &texUpdatedDirectionsLines);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, texUpdatedDirectionsLines);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size_t(lineBufferSize) * 3 * sizeof(float), NULL, GL_STATIC_DRAW);

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

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	//free temporarily used memory on HOST side
	trackId.clear();            
	isFiberBegin.clear();
	isFiberEnd.clear();
	resampledTrackOffset.clear();
	resampledTrackSize.clear();
}

void FiberBundler::destroyTextures() {
	glDeleteBuffers(1, &texTempTracks);
	glDeleteBuffers(1, &texUpdatedTracks);
	glDeleteBuffers(1, &texSmoothedTracks);
	glDeleteBuffers(1, &texRelaxedTracks);
	glDeleteBuffers(1, &texVoxelCount);
	glDeleteBuffers(1, &texIsFiberBegin);
	glDeleteBuffers(1, &texIsFiberEnd);
	glDeleteBuffers(1, &texTrackId);
	glDeleteBuffers(1, &texRelaxedLines);

	glDeleteTextures(1, &texDenseX);
	glDeleteTextures(1, &texDenseY);
	glDeleteTextures(1, &texDenseMap);
}

void FiberBundler::cleanHostMemory() {
	trackId.clear();
	isFiberBegin.clear();
	isFiberEnd.clear();
	resampledTrackOffset.clear();
	resampledTrackSize.clear();
	resampledTracks.clear();
	resampledDirections.clear();
}

bool FiberBundler::isEnabled() {
	return isEnable;
}

void FiberBundler::edgeBundlingGPU(float _p) {
	bundle = _p;
	//repeat bundling for nIter iterations

	//init texTempTracks with original tracks (resampled)
	glBindBuffer(GL_TEXTURE_BUFFER, texTempTracks);
	glBufferData(GL_TEXTURE_BUFFER, resampledTracks.size() * 3 * sizeof(float), resampledTracks.data(), GL_STATIC_DRAW);

	//init texTempDirections with original direction (resampled)
	glBindBuffer(GL_TEXTURE_BUFFER, texTempDirections);
	glBufferData(GL_TEXTURE_BUFFER, resampledDirections.size() * 3 * sizeof(float), resampledDirections.data(), GL_STATIC_DRAW);

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
		glBufferData(GL_TEXTURE_BUFFER, resampledTracks.size() * 3 * sizeof(float), resampledTracks.data(), GL_STATIC_DRAW);
		relaxationPass();
		updateDirectionPass();

		//copy data from resulting updated to temp for next iteration
		transferDataGPU(texRelaxedTracks, texTempTracks, resampledTracks.size() * 3 * sizeof(float));
		transferDataGPU(texUpdatedDirections, texTempDirections, resampledDirections.size() * 3 * sizeof(float));

		//slightly decrease the kernel width to adapt to the increasingly bundled fibers
		p *= 0.95;
	}

	//map from tracks to lines for rendering
	trackToLinePass(texRelaxedTracks, texRelaxedLines);   
	trackToLinePass(texUpdatedDirections, texUpdatedDirectionsLines);

	//update vertex and direction buffer of the instance
	updateInstanceFibers();
}

//resampledTracks->voxelCount
void FiberBundler::voxelCountPass() {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texTempTracks);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, texVoxelCount);

	voxelCountShader->use();
	voxelCountShader->setInt("totalSize", resampledTracks.size());
	voxelCountShader->setInt("nVoxels_X", nVoxels_X);
	voxelCountShader->setInt("nVoxels_Y", nVoxels_Y);
	voxelCountShader->setInt("nVoxels_Z", nVoxels_Z);
	voxelCountShader->setVec3("aabbMin", aabb.minPos);
	voxelCountShader->setFloat("voxelUnitSize", voxelUnitSize);
	voxelCountShader->setInt("totalVoxels", totalVoxels);

	glDispatchCompute(1 + (unsigned int)resampledTracks.size() / 128, 1, 1);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

//voxelVount->denseMap
void FiberBundler::denseEstimationPass(float p) {
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
void FiberBundler::advectionPass(float p) {
	glBindImageTexture(0, texDenseMap, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texTempTracks);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, texUpdatedTracks);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, texTempDirections);

	advectionShader->use();
	advectionShader->setInt("totalSize", resampledTracks.size());
	advectionShader->setInt("nVoxels_X", nVoxels_X);
	advectionShader->setInt("nVoxels_Y", nVoxels_Y);
	advectionShader->setInt("nVoxels_Z", nVoxels_Z);
	advectionShader->setInt("kernelR", p * nVoxels_Z);
	advectionShader->setFloat("voxelUnitSize", voxelUnitSize);
	advectionShader->setVec3("aabbMin", aabb.minPos);
	advectionShader->setInt("totalVoxels", totalVoxels);
	glDispatchCompute(1 + (unsigned int)resampledTracks.size() / 128, 1, 1);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	//glBindBuffer(GL_ARRAY_BUFFER, debugFLOAT);
	//float * debugData = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
	//for (int i = 0;i<6 *tubes.size(); i++) {
	//	std::cout << debugData[i];
	//}
}

void FiberBundler::smoothPass(float p) {
	int smoothL = p * nVoxels_Z;

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texUpdatedTracks);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, texSmoothedTracks);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, texIsFiberBegin);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, texIsFiberEnd);

	smoothShader->use();
	smoothShader->setInt("totalSize", resampledTracks.size());
	smoothShader->setFloat("smoothFactor", smoothFactor);
	smoothShader->setInt("smoothL", smoothL);
	smoothShader->setFloat("voxelUnitSize", voxelUnitSize);

	glDispatchCompute(1 + (unsigned int)resampledTracks.size() / 128, 1, 1);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void FiberBundler::relaxationPass() {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texTempTracks);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, texSmoothedTracks);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, texRelaxedTracks);

	relaxShader->use();
	relaxShader->setInt("totalSize", resampledTracks.size());
	relaxShader->setFloat("relaxFactor", relaxFactor);

	glDispatchCompute(1 + (unsigned int)resampledTracks.size() / 128, 1, 1);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void FiberBundler::updateDirectionPass() {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texRelaxedTracks);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, texUpdatedDirections);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, texIsFiberBegin);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, texIsFiberEnd);

	updateDirectionShader->use();
	updateDirectionShader->setInt("totalSize", resampledDirections.size());

	glDispatchCompute(1 + (unsigned int)resampledDirections.size() / 128, 1, 1);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}


//map points in tracks to lines (double repeat the interior points in a track)
void FiberBundler::trackToLinePass(GLuint texTracks, GLuint texLines) {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, texTracks);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, texLines);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, texIsFiberBegin);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, texIsFiberEnd);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, texTrackId);

	trackToLinesShader->use();
	trackToLinesShader->setInt("totalSize", resampledTracks.size());

	glDispatchCompute(1 + (unsigned int)resampledTracks.size() / 128, 1, 1);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}


void FiberBundler::transferDataGPU(GLuint srcBuffer, GLuint dstBuffer, size_t copySize) {
	glBindBuffer(GL_ARRAY_BUFFER, srcBuffer);
	glBindBuffer(GL_COPY_WRITE_BUFFER, dstBuffer);

	// Transfer data from buffer 1 to buffer 2
	glCopyBufferSubData(GL_ARRAY_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, copySize);

	//For synchronization
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// Unbind buffers
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
}

int FiberBundler::getLineBufferSize() {
	return lineBufferSize;
}

float FiberBundler::getBundleValue() {
	return bundle;
}

// update the activated instance with bundled fibers
void FiberBundler::updateInstanceFibers() {
	instance->updateVertexBuffer(texRelaxedLines, size_t(lineBufferSize) * 3 * sizeof(float));
	instance->updateDirectionBuffer(texUpdatedDirectionsLines, size_t(lineBufferSize) * 3 * sizeof(float));
	instance->setNowLineBufferSize(lineBufferSize);
}

GLuint FiberBundler::getBundledFibers() {
	return texRelaxedLines;
}

//called when user switches to another instance
void FiberBundler::switcheToInstance(Instance* _instance) {
	//clean up first
	destroyTextures();
	cleanHostMemory();

	//reInit
	instance = _instance;
	initForInstance();

	//perform initial bundling for the currently activated instance
	float p = instance->getBundleValue();
	bundle = p;
	if (p > 0) {
		edgeBundlingGPU(p);
	}
}