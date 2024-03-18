#include"Scene.h"

Scene::Scene() {

}

Scene::~Scene() {

}

void Scene::addInstance(std::string filePath) {
	instances.push_back(Instance(filePath, radius, nTris));
}

void Scene::setActivatedInstance(int id) {
	activatedInstance = id;
	instances.at(id).initSSBOBinding();
}

std::vector<Vertex>& Scene::getInstanceVertices(int insId) {
	return instances.at(insId).getVertices();
}

std::vector<uint32_t>& Scene::getInstanceIndicies(int insId) {
	return instances.at(insId).getIndices();
}

void Scene::drawActivatedInstanceLineMode(float lineWidth) {
	instances.at(activatedInstance).drawLineMode(lineWidth);
}

void Scene::drawAllInstancesLineMode(float lineWidth) {
	for (Instance& instance : instances) {
		instance.drawLineMode(lineWidth);
	}
}

void Scene::setRadius(float r) {
	radius = r;
	updateMeshNewRadius();
	
}

void Scene::setNTris(int n) {
	nTris = n;
	updateMeshNewNTris();
}

void Scene::updateMeshNewRadius() {
	for (Instance& instance : instances) {
		instance.recreateMeshNewRadius(radius, nTris);
	}
}

void Scene::updateMeshNewNTris() {
	for (Instance& instance : instances) {
		instance.recreateMeshNewNTris(radius, nTris);
	}
}

void Scene::edgeBundling(float p, float radius, int nTris) {
	//for (Instance& instance : instances) {
	//	//instance.edgeBundling(p,radius,nTris);
	//	instance.edgeBundlingGPU(p, radius, nTris);
	//}
	instances.at(activatedInstance).edgeBundlingGPU(p, radius, nTris);
	//instances.at(activatedInstance).testSmoothing();
	//instances.at(activatedInstance).edgeBundlingCUDA(p, radius, nTris);
}

int Scene::getInstanceNVoxelsX() {
	return instances.at(activatedInstance).getNVoxelsX();
}

int Scene::getInstanceNVoxelsY() {
	return instances.at(activatedInstance).getNVoxelsY();
}

int Scene::getInstanceNVoxelsZ() {
	return instances.at(activatedInstance).getNVoxelsZ();
}

glm::vec3 Scene::getInstanceAABBMin() {
	return instances.at(activatedInstance).getAABBMin();
}

float Scene::getInstanceVoxelUnitSize() {
	return instances.at(activatedInstance).getVoxelUnitSize();
}

int Scene::getInstanceTotalVoxels() {
	return instances.at(activatedInstance).getTotalVoxels();
}

GLuint Scene::getInstanceDenseMap() {
	return instances.at(activatedInstance).getDenseMap();
}

GLuint Scene::getInstanceVoxelCount() {
	return instances.at(activatedInstance).getVoxelCount();
}

void Scene::slicing(glm::vec3 pos, glm::vec3 dir) {
	instances.at(activatedInstance).slicing(pos, dir);
}

void Scene::updateInstanceEnableSlicing() {
	instances.at(activatedInstance).updateEnableSlicing();
}

