#include"Scene.h"

Scene::Scene() {

}

Scene::~Scene() {

}

void Scene::initComputeShaders() {
	denseEstimationShaderX = new ComputeShader("shaders/denseEstimationX.cs") ;
	denseEstimationShaderY = new ComputeShader("shaders/denseEstimationY.cs");
	denseEstimationShaderZ = new ComputeShader("shaders/denseEstimationZ.cs");
	advectionShader = new ComputeShader("shaders/advection.cs");
	voxelCountShader = new ComputeShader("shaders/voxelCount.cs");
	smoothShader = new ComputeShader("shaders/smooth.cs");
	relaxShader = new ComputeShader("shaders/relaxation.cs");
	updateDirectionShader = new ComputeShader("shaders/updateDirections.cs");
	updateNormalShader = new ComputeShader("shaders/updateNormals.cs");
	forceConsecutiveShader = new ComputeShader("shaders/forceConsecutive.cs");
	slicingShader = new ComputeShader("shaders/slicing.cs");
}

void Scene::addInstance(std::string filePath) {
	instances.push_back(Instance(
		filePath,
		std::make_shared<ComputeShader>(*voxelCountShader),
		std::make_shared<ComputeShader>(*denseEstimationShaderX),
		std::make_shared<ComputeShader>(*denseEstimationShaderY),
		std::make_shared<ComputeShader>(*denseEstimationShaderZ),
		std::make_shared<ComputeShader>(*advectionShader),
		std::make_shared<ComputeShader>(*smoothShader),
		std::make_shared<ComputeShader>(*relaxShader),
		std::make_shared<ComputeShader>(*updateDirectionShader),
		std::make_shared<ComputeShader>(*updateNormalShader),
		std::make_shared<ComputeShader>(*forceConsecutiveShader),
		std::make_shared<ComputeShader>(*slicingShader)
	));
}

void Scene::removeInstance(int insId) {
	if(insId < instances.size())
	  instances.erase(instances.begin());
	if (instances.size() > 0)
		activatedInstance = 0;
	else
		activatedInstance = -1;
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
	if(instances.size()>0)
		instances.at(activatedInstance).drawLineMode(lineWidth);
}

void Scene::drawAllInstancesLineMode(float lineWidth) {
	for (Instance& instance : instances) {
		instance.drawLineMode(lineWidth);
	}
}

//void Scene::setRadius(float r) {
//	radius = r;
//	updateMeshNewRadius();
//	
//}
//
//void Scene::setNTris(int n) {
//	nTris = n;
//	updateMeshNewNTris();
//}

//void Scene::updateMeshNewRadius() {
//	for (Instance& instance : instances) {
//		instance.recreateMeshNewRadius(radius, nTris);
//	}
//}
//
//void Scene::updateMeshNewNTris() {
//	for (Instance& instance : instances) {
//		instance.recreateMeshNewNTris(radius, nTris);
//	}
//}

void Scene::edgeBundling(float p, float radius, int nTris) {
	//for (Instance& instance : instances) {
	//	//instance.edgeBundling(p,radius,nTris);
	//	instance.edgeBundlingGPU(p, radius, nTris);
	//}
	if (instances.size() > 0)
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
	if (instances.size() > 0)
		instances.at(activatedInstance).slicing(pos, dir);
}

void Scene::updateInstanceEnableSlicing(glm::vec3 pos, glm::vec3 dir) {
	if (instances.size() > 0)
		instances.at(activatedInstance).updateEnableSlicing(pos, dir);
}

void Scene::setInstanceMaterial(float roughness, float metallic) {
	if (instances.size() > 0)
		instances.at(activatedInstance).setMaterial(roughness, metallic);
}

void Scene::getInstanceMaterial(float* roughness, float* metallic) {
	if (instances.size() > 0)
		instances.at(activatedInstance).getMaterial(roughness, metallic);
}

bool Scene::isEmpty() {
	return instances.size() == 0;
}

