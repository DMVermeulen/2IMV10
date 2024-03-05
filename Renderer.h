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
	void setContrast(float contrast);
private:
	Scene* scene = nullptr; 
	Camera* camera = nullptr;
	int WIDTH;
	int HEIGHT;

	void initGeoPassObjects();
	void initShadingPassObjects();
	void initSSAOPassObjects();
	void initPostPassObjects();
	void initTexVisPassObjects();

	void geometryPass();
	void shadingPass();
	void ssaoPass();
	void postProcessingPass();
	void renderQuad();
	void textureVisPass();

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
	GLuint framebufferSSAOPass;
	GLuint colorBufferSSAOPass;
	std::unique_ptr<Shader> ssaoPassShader;

	//For postProcessing pass
	std::unique_ptr<Shader> postPassShader;

	//For texture visualization pass
	std::unique_ptr<Shader> texVisPassShader;

	//viewport size
	GLuint width, height;

	//UI parameters
	float ssaoRadius=10;
	float lineWidth = 1.0f;
	float colorInterval = 0;
	float contrast = 1.0f;

	//commonly used helper function (may moved to class RenderPassBase)
	void createQuadObjects();

};