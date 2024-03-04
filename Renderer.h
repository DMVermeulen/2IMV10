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
	void setLineWidth(float lineWidth);
	void setSSAORadius(float R);
	void setColorFlattening(float colorInterval);
private:
	Scene* scene = nullptr; 
	Camera* camera = nullptr;
	int WIDTH;
	int HEIGHT;

	void initGeoPassObjects();
	void initShadingPassObjects();
	void initSSAOPassObjects();
	void initPostPassObjects();

	void geometryPass();
	void shadingPass();
	void ssaoPass();
	void postProcessingPass();
	void renderQuad();

	//helper functions
	float ourLerp(float a, float b, float f);

	//OpenGL related 
	//For geometry pass
	GLuint gBuffer;
	GLuint gPos;           
	GLuint gNormal;
	GLuint gDir;
	GLuint depthMap;
	std::unique_ptr<Shader> geoPassShader;

	//For quad drawing
	GLuint quadVAO, quadVBO, quadIBO, quadTex;

	//For shading pass
	GLuint framebufferShadingPass;
	GLuint colorBufferShadingPass;
	std::unique_ptr<Shader> lightPassShader;

	//For SSAO pass
	std::vector<glm::vec3> ssaoKernel;
	GLuint noiseTexture;
	std::unique_ptr<Shader> ssaoPassShader;

	//For postProcessing pass

	//viewport size
	GLuint width, height;

	//UI parameters
	float ssaoRadius=10;
	float lineWidth = 1.0f;
	float colorInterval = 0;

	//commonly used helper function (may moved to class RenderPassBase)
	void createQuadObjects();

};