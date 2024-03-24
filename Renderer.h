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
	void setBrightness(float brightness);
	void setSaturation(float saturation);
	void setSharpening(float sharpening);
	void updateShadingPassInstanceInfo();
	void setRenderMode(int renderMode);
	void setLightingMode(int lightingMode);
	void setColorMode(int colorMode);
	void setColorConstant(glm::vec3 constant);
	void updateViewportSize(int width, int height);
private:
	Scene* scene = nullptr; 
	Camera* camera = nullptr;
	//int WIDTH;
	//int HEIGHT;

	void initGeoPassObjects();
	void initShadingPassObjects();
	void initSSAOPassObjects();
	void initPostPassObjects();
	void initSharpeningObjects();
	void initTexVisPassObjects();
	void recreateObjects();

	void updateFrame();
	void geometryPass();
	void shadingPass();
	void ssaoPass();
	void postProcessingPass();
	void sharpeningPass();
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
	GLuint gMotionVector;
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
	GLuint framebufferPostPass;
	GLuint colorBufferPostPass;

	//For motion bluring
	glm::mat4 lastCameraView; //record the view matrix from last frame, used for motion blurring
	glm::mat4 cameraView;

	//For sharpening pass
	std::unique_ptr<Shader> sharpeningPassShader;

	//For texture visualization pass
	std::unique_ptr<Shader> texVisPassShader;

	//viewport size
	int width = -1;
	int height=-1;

	//UI parameters
	float ssaoRadius=10;
	float lineWidth = 1.0f;
	float colorInterval = 0;
	float contrast = 1.0f;
	float brightness = 0.0f;
	float saturation = 1.0f;
	float sharpening = 0.0f;
	int renderMode = 0;
	int lightingMode = 0;
	int colorMode = 0;
	glm::vec3 colorConstant = glm::vec3(0, 0, 1);

	//commonly used helper function (may moved to class RenderPassBase)
	void createQuadObjects();

};