#include"Renderer.h"
#include<iostream>
#include"glm/gtc/matrix_transform.hpp"

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
		"D:/Projects/FiberVisualization/shaders/geoPassVertex.glsl",
		"D:/Projects/FiberVisualization/shaders/geoPassFragment.glsl")
		);

	lightPassShader = std::unique_ptr< Shader >(new Shader(
		"D:/Projects/FiberVisualization/shaders/lightPassVertex.glsl",
		"D:/Projects/FiberVisualization/shaders/lightPassFragment.glsl")
		);
	//init objects for all render passes
	initGeoPassObjects();
	initShadingPassObjects();
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
	postProcessingPass();
}

void Renderer::initGeoPassObjects() {
	//generate framebuffer for geopass
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

	//bind texture gPos that will be written by geopass
	glGenTextures(1, &gPos);
	glBindTexture(GL_TEXTURE_2D, gPos);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPos, 0);

	GLuint attachments[1] = { GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, attachments);

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

}

void Renderer::initPostPassObjects() {

}

void Renderer::geometryPass() {
	glEnable(GL_DEPTH_TEST);
	// select framebuffer as render target
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// pass transformation matrices to shader, remember to enable shader before setting uniform values!!!
	geoPassShader->enable();
	glm::mat4 proj = glm::perspective(glm::radians(camera->Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 500.0f);
	glUniformMatrix4fv(glGetUniformLocation(geoPassShader->getProgramId(), "proj"), 1, GL_FALSE, &proj[0][0]);
	glm::mat4 view = camera->GetViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(geoPassShader->getProgramId(), "view"), 1, GL_FALSE, &view[0][0]);

	scene->drawAllInstances(); //scene will submit the drawing commands for each instance 

	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	geoPassShader->disable();
}

void Renderer::shadingPass() {
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	lightPassShader->enable();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPos);
	glUniform1i(glGetUniformLocation(lightPassShader->getProgramId(), "worldPos"), 0);

	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIBO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	lightPassShader->disable();
}

void Renderer::postProcessingPass() {

}
