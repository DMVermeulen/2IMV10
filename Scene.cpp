#include"Scene.h"

Scene::Scene() {

}

Scene::~Scene() {
	delete slicingShader;
	delete trackToLinesShader;
}

void Scene::initComputeShaders() {
	slicingShader = new ComputeShader("shaders/slicing.cs");
	trackToLinesShader = new ComputeShader("shaders/trackToLines.cs");

	bundler.init();
}

void Scene::addInstance(std::string filePath) {
	instances.push_back(Instance(
		filePath,
		std::make_shared<ComputeShader>(*slicingShader),
		std::make_shared<ComputeShader>(*trackToLinesShader)
	));
}

void Scene::removeInstance(int insId) {
	if(insId < instances.size())
	  instances.erase(instances.begin()+insId);
	if (instances.size() > 0)
		activatedInstance = 0;
	else
		activatedInstance = -1;
}

void Scene::setActivatedInstance(int id) {
	int preId = activatedInstance;
	if(preId >=0 && preId <instances.size())
	 instances.at(preId).deactivate();
		
	activatedInstance = id;
	if (id >= 0 && id < instances.size()) {
		instances.at(id).activate();
		if (bundler.isEnabled()) {
			bundler.switcheToInstance(&(instances.at(id)));
		}
	}
}

void Scene::drawActivatedInstanceLineMode(float lineWidth) {
	if (instances.size() > 0) {
		Instance& activeInstance = instances.at(activatedInstance);
		if(bundler.isEnabled())
			activeInstance.drawLineMode(lineWidth,bundler.getLineBufferSize());
		else
			activeInstance.drawLineMode(lineWidth,activeInstance.getOriLineBufferSize());
	}
}

void Scene::updateFiberBundlingStatus(bool enable) {
	if (instances.empty())
		return;
	Instance& activeInstance = instances.at(activatedInstance);
	if (enable)
		bundler.enable(&activeInstance);
	else {
		bundler.disable();
		activeInstance.setBundleValue(0);
		activeInstance.restoreOriginalLines();
		if (activeInstance.isSlicingEnabled()) {
			activeInstance.slicing(activeInstance.getOriFibers());
		}
	}
}

void Scene::edgeBundling(float p) {
	//if (instances.size() > 0)
	//	instances.at(activatedInstance).edgeBundlingGPU(p);
	if (instances.size() > 0 && bundler.isEnabled()) {
		bundler.edgeBundlingGPU(p);
		instances.at(activatedInstance).setBundleValue(p);
	}
}

glm::vec3 Scene::getInstanceAABBMin() {
	return instances.at(activatedInstance).getAABBMin();
}

void Scene::slicing(glm::vec3 pos, glm::vec3 dir) {
	if (instances.size() > 0) {
		Instance& activeInstance = instances.at(activatedInstance);
		activeInstance.setSlicingPlane(pos, dir);
		GLuint slicingSource;
		if (bundler.isEnabled() && bundler.getBundleValue() > 0)
			slicingSource = bundler.getBundledFibers();
		else
			slicingSource = activeInstance.getOriFibers();
		activeInstance.slicing(slicingSource);
	}
}

void Scene::updateInstanceEnableSlicing(glm::vec3 pos, glm::vec3 dir, bool enable) {
	//if (instances.size() > 0)
	//	instances.at(activatedInstance).updateEnableSlicing(pos, dir);
	if (instances.size() > 0) {
		Instance& activeInstance = instances.at(activatedInstance);
		activeInstance.setEnableSlicing(enable);
		if (enable) {
			activeInstance.setSlicingPlane(pos, dir);
			GLuint slicingSource;
			if (bundler.isEnabled() && bundler.getBundleValue() > 0)
				slicingSource = bundler.getBundledFibers();
			else
				slicingSource = activeInstance.getOriFibers();
			activeInstance.slicing(slicingSource);
		}
		else {
			if (bundler.isEnabled() && bundler.getBundleValue() > 0) {
				bundler.updateInstanceFibers();
			}
			else
				activeInstance.restoreOriginalLines();
		}
	}
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

// return instance settings to GUI when switching instance
void Scene::getInstanceSettings(float* bundle, bool* enableSlicing, glm::vec3* slicePos, glm::vec3* sliceDir) {
	instances.at(activatedInstance).getSettings(bundle, enableSlicing,slicePos, sliceDir);
}

