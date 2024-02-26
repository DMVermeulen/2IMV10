#include"Scene.h"

Scene::Scene() {

}

Scene::~Scene() {

}

void Scene::addInstance(std::string filePath) {
	instances.push_back(Instance(filePath, radius, nTris));
}

std::vector<Vertex>& Scene::getInstanceVertices(int insId) {
	return instances.at(insId).getVertices();
}

std::vector<uint32_t>& Scene::getInstanceIndicies(int insId) {
	return instances.at(insId).getIndices();
}

void Scene::drawAllInstances() {
	for (Instance& instance : instances) {
		instance.draw();
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
	for (Instance& instance : instances) {
		instance.edgeBundling(p,radius,nTris);
	}
}

