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

