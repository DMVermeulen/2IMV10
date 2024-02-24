#pragma once
#include"glad/glad.h"
#include"Scene.h"
#include"Shader.h"
#include"Camera.h"

class Renderer {
public:
	Renderer();
	~Renderer();

	void renderFrame();
	void setScene(Scene* scene);
	void setCamera(Camera* camera);
	void setViewportSize(int width, int height);
	void init();
private:
	Scene* scene = nullptr; 
	Camera* camera = nullptr;
	int WIDTH;
	int HEIGHT;

	void initGeoPassObjects();
	void initShadingPassObjects();
	void initPostPassObjects();

	void geometryPass();
	void shadingPass();
	void postProcessingPass();

	//OpenGL related 
	//For geometry pass
	GLuint gBuffer;
	GLuint gPos; //produced by geometryPass, can be extended to more
	GLuint depthMap;
	std::unique_ptr<Shader> geoPassShader;

	//For shading pass
	GLuint quadVAO, quadVBO, quadIBO, quadTex;
	std::unique_ptr<Shader> lightPassShader;

	//For postProcessing pass

	//viewport size
	GLuint width, height;

	//commonly used helper function (may moved to class RenderPassBase)
	void createQuadObjects();

};