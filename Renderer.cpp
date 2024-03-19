#include"Renderer.h"
#include<iostream>
#include"glm/gtc/matrix_transform.hpp"
#include <random>

Renderer::Renderer() {

}

Renderer::~Renderer() {

}

void Renderer::init() {
	//query for viewport size
	GLint value[4];
	glGetIntegerv(GL_VIEWPORT, value);
	width = value[2];
	height = value[3];
	//load shaders
	geoPassShader = std::unique_ptr< Shader >(new Shader(
		"shaders/geoPassVertex.glsl",
		"shaders/geoPassFragment.glsl")
		);

	lightPassShader = std::unique_ptr< Shader >(new Shader(
		"shaders/fullQuad.glsl",
		"shaders/lightPassFragment.glsl")
		);

	ssaoPassShader = std::unique_ptr< Shader >(new Shader(
		"shaders/fullQuad.glsl",
		"shaders/ssaoPassFragment.glsl")
		);
	texVisPassShader = std::unique_ptr< Shader >(new Shader(
		"shaders/fullQuad.glsl",
		"shaders/texVisPassFragment.glsl")
		);
	postPassShader = std::unique_ptr< Shader >(new Shader(
		"shaders/fullQuad.glsl",
		"shaders/postPassFragment.glsl")
		);
	sharpeningPassShader = std::unique_ptr< Shader >(new Shader(
		"shaders/fullQuad.glsl",
		"shaders/sharpeningPassFragment.glsl")
		);

	//init objects for all render passes
	initGeoPassObjects();
	initShadingPassObjects();
	initSSAOPassObjects();
	initPostPassObjects();
	createQuadObjects();
}

void Renderer::setScene(Scene* _scene) {
	scene = _scene;
}

void Renderer::setCamera(Camera* _camera) {
	camera = _camera;
}

void Renderer::setViewportSize(int width, int height) {
	WIDTH = width;
	HEIGHT = height;
}

void Renderer::renderFrame() {
	geometryPass();
	shadingPass();
	ssaoPass();
	//textureVisPass();
	postProcessingPass();
	sharpeningPass();
}

void Renderer::initGeoPassObjects() {
	//generate framebuffer for geopass
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

	//bind textures that will be written by geopass
	glGenTextures(1, &gPos);
	glBindTexture(GL_TEXTURE_2D, gPos);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPos, 0);

	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

	glGenTextures(1, &gDir);
	glBindTexture(GL_TEXTURE_2D, gDir);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gDir, 0);

	GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	//bind depth map for depth testing
	glGenRenderbuffers(1, &depthMap);
	glBindRenderbuffer(GL_RENDERBUFFER, depthMap);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthMap);

	if (GL_FRAMEBUFFER_COMPLETE!=glCheckFramebufferStatus(GL_FRAMEBUFFER))
		std::cout << "Incomplete framebuffer! " << std::endl;

	//unbind framebuffer after setting up
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::createQuadObjects() {
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glGenBuffers(1, &quadIBO);
	glGenBuffers(1, &quadTex);

	const GLfloat quadVertices[] = {
		-1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f
	};

	const GLfloat texCoords[] = {
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f
	};

	const GLuint quadIndices[] = {
		0, 1, 2,
		0, 2, 3
	};


	glBindVertexArray(quadVAO);

	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, quadTex);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

	glBindVertexArray(0);
}

void Renderer::initShadingPassObjects() {
	glGenFramebuffers(1, &framebufferShadingPass);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferShadingPass);

	glGenTextures(1, &colorBufferShadingPass);
	glBindTexture(GL_TEXTURE_2D, colorBufferShadingPass);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferShadingPass, 0);

	GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "shading Framebuffer not complete!" << std::endl;
}

void Renderer::initSSAOPassObjects() {
	glGenFramebuffers(1, &framebufferSSAOPass);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferSSAOPass);

	glGenTextures(1, &colorBufferSSAOPass);
	glBindTexture(GL_TEXTURE_2D, colorBufferSSAOPass);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferSSAOPass, 0);

	GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "shading Framebuffer not complete!" << std::endl;

	// generate sample kernel for SSAO
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;
	for (unsigned int i = 0; i < 64; ++i)
	{
		glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / 64.0f;

		// scale samples s.t. they're more aligned to center of kernel
		scale = ourLerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel.push_back(sample);
		
		float length = glm::sqrt(glm::dot(sample, sample));
	}

	//generate random vectors for sampling in SSAO pass
	std::vector<glm::vec3> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
		ssaoNoise.push_back(noise);
	}
	unsigned int noiseTexture; glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//set texture location for samplers
	ssaoPassShader->enable();
	ssaoPassShader->setInt("shadedColor", 0);
	ssaoPassShader->setInt("gPosition", 1);
	ssaoPassShader->setInt("gNormal", 2);
	ssaoPassShader->setInt("gDir", 3);
	ssaoPassShader->setInt("texNoise", 4);
	ssaoPassShader->disable();
}

void Renderer::initPostPassObjects() {
	glGenFramebuffers(1, &framebufferPostPass);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferPostPass);

	glGenTextures(1, &colorBufferPostPass);
	glBindTexture(GL_TEXTURE_2D, colorBufferPostPass);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferPostPass, 0);

	GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "shading Framebuffer not complete!" << std::endl;

	postPassShader->enable();
	postPassShader->setInt("imageWithAO", 0);
	postPassShader->disable();
}

void Renderer::initSharpeningObjects() {
	sharpeningPassShader->enable();
	sharpeningPassShader->setInt("imageData", 0);
	sharpeningPassShader->disable();
}

void Renderer::initTexVisPassObjects() {
	//set texture location for samplers
	texVisPassShader->enable();
	texVisPassShader->setInt("sampleTexture", 0);
	texVisPassShader->disable();
}

void Renderer::geometryPass() {
	glEnable(GL_DEPTH_TEST);
	// select framebuffer as render target
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// pass transformation matrices to shader, remember to enable shader before setting uniform values!!!
	geoPassShader->enable();
	glm::mat4 proj = glm::perspective(glm::radians(camera->Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
	glUniformMatrix4fv(glGetUniformLocation(geoPassShader->getProgramId(), "proj"), 1, GL_FALSE, &proj[0][0]);
	glm::mat4 view = camera->GetViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(geoPassShader->getProgramId(), "view"), 1, GL_FALSE, &view[0][0]);

	geoPassShader->setVec3("viewPos", camera->Position);

	//scene->drawAllInstancesLineMode(lineWidth); //scene will submit the drawing commands for each instance 
	scene->drawActivatedInstanceLineMode(lineWidth);

	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	geoPassShader->disable();
}

void Renderer::shadingPass() {
	glDisable(GL_DEPTH_TEST);
	// select framebuffer as render target
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferShadingPass);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	lightPassShader->enable();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPos);
	glUniform1i(glGetUniformLocation(lightPassShader->getProgramId(), "worldPos"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gDir);
	glUniform1i(glGetUniformLocation(lightPassShader->getProgramId(), "gDir"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glUniform1i(glGetUniformLocation(lightPassShader->getProgramId(), "gNormal"), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_3D, scene->getInstanceDenseMap());
	glUniform1i(glGetUniformLocation(lightPassShader->getProgramId(), "denseMap"), 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_3D, scene->getInstanceVoxelCount());
	glUniform1i(glGetUniformLocation(lightPassShader->getProgramId(), "voxelCount"), 4);

	lightPassShader->setVec3("viewPos",camera->Position);
	lightPassShader->setInt("colorMode", this->renderMode);
	float rough, metal;
	scene->getInstanceMaterial(&rough, &metal);
	lightPassShader->setFloat("roughness",rough);
	lightPassShader->setFloat("metallic", metal);
	lightPassShader->setInt("lightingMode", lightingMode);
	lightPassShader->setInt("colorMode", colorMode);
	lightPassShader->setVec3("constantAlbedo", colorConstant);

	renderQuad();

	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	lightPassShader->disable();
}

void Renderer::ssaoPass() {
	glDisable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferSSAOPass);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	ssaoPassShader->enable();

	//textures to sample from
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorBufferShadingPass);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gPos);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gNormal);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gDir);
		
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, noiseTexture); 

	//set uniforms
	for (unsigned int i = 0; i < 64; ++i)
		ssaoPassShader->setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
	glm::mat4 proj = glm::perspective(glm::radians(camera->Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
	glm::mat4 view = camera->GetViewMatrix();
	ssaoPassShader->setFloat("colorInterval", colorInterval);
	ssaoPassShader->setFloat("radius", ssaoRadius);
	ssaoPassShader->setMat4("view",view);
	ssaoPassShader->setMat4("proj", proj);
	ssaoPassShader->setVec3("viewPos", camera->Position);

	renderQuad();

	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ssaoPassShader->disable();
}

void Renderer::postProcessingPass() {

	//contrast + illuminance
	glDisable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferPostPass);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	postPassShader->enable();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorBufferSSAOPass);
	postPassShader->setFloat("contrast", contrast);
	postPassShader->setFloat("brightness", brightness);
	renderQuad();
	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	postPassShader->disable();
}

void Renderer::sharpeningPass() {
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	sharpeningPassShader->enable();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorBufferPostPass);
	sharpeningPassShader->setFloat("sharpening", sharpening);
	renderQuad();
	glBindVertexArray(0);
	sharpeningPassShader->disable();
}

void Renderer::textureVisPass() {
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	texVisPassShader->enable();

	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, gPos);

	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, gNormal);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gDir);

	renderQuad();

	glBindVertexArray(0);
	texVisPassShader->disable();
}

void Renderer::renderQuad() {
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIBO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

float Renderer::ourLerp(float a, float b, float f)
{
	return a + f * (b - a);
}

void Renderer::setSSAORadius(float R) {
	ssaoRadius = R;
}

void Renderer::setLineWidth(float _lineWidth) {
	lineWidth = _lineWidth;
}

void Renderer::setColorFlattening(float _colorInterval) {
	colorInterval = _colorInterval;
}

void Renderer::setContrast(float _contrast) {
	contrast = _contrast;
}

void Renderer::setBrightness(float _brightness) {
	brightness = _brightness;
}

void Renderer::setSharpening(float _sharpening) {
	sharpening = _sharpening;
}

void Renderer::updateShadingPassInstanceInfo() {
	lightPassShader->enable();
	lightPassShader->setInt("nVoxels_X", scene->getInstanceNVoxelsX());
	lightPassShader->setInt("nVoxels_Y", scene->getInstanceNVoxelsY());
	lightPassShader->setInt("nVoxels_Z", scene->getInstanceNVoxelsZ());
	lightPassShader->setVec3("aabbMin", scene->getInstanceAABBMin());
	lightPassShader->setFloat("voxelUnitSize", scene->getInstanceVoxelUnitSize());
	lightPassShader->setInt("totalVoxels", scene->getInstanceTotalVoxels());
	lightPassShader->disable();
}

void Renderer::setRenderMode(int _mode) {
	renderMode = _mode;
}

void Renderer::setLightingMode(int _lightingMode) {
	lightingMode = _lightingMode;
}

void Renderer::setColorMode(int _colorMode) {
	colorMode = _colorMode;
}

void Renderer::setColorConstant(glm::vec3 constant) {
	colorConstant = constant;
}