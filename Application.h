#pragma once
#include<string>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include"Renderer.h"
#include"Scene.h"
#include"Camera.h"
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include"imfilebrowser.h"

class Application {
public:
	Application();
	~Application();
	void run();
	static Application* m_app;  	//self pointer (used by static method)
	//UI operations
private:
	bool suspendRender=false;
	//UI related
	ImGuiIO io;
	ImGui::FileBrowser fileDialog;
	bool show_demo_window = true;
	bool show_another_window = false;
	bool ImGuiIOWantCaptureMouse = false;
	bool ImGuiIOWantCaptureKeyboard = false;
	ImVec4 clear_color = ImVec4(0, 0, 0, 1.00f);
	int SCR_WIDTH = 1280;
	int SCR_HEIGHT = 720;
	//A three-pass render
	Renderer renderer;
	//Scene shared by possibly multiple renderers
	Scene scene;
	//Camera related
	Camera camera;
	float deltaTime = 0.0f;
	float lastFrame = 0.0f;
	float lastX;
	float lastY;
	bool firstMouse = true;

	//UI related
	const int settingsPanelWidth = 300;
	ImVec2 settingsPanelPos;
	ImVec2 settingsPanelSize;

	GLFWwindow* window;
	void initWindow();
	void initScene();
	void initRenderer();
	void mainLoop();
	void renderFrame();
	void processInput(GLFWwindow *window);
	static void window_size_update_callback(GLFWwindow* window, int width, int height);
	static void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	static void window_iconify_callback(GLFWwindow* window, int iconified);

	//UI related
	void renderUI();
	void renderSettingPanel();
	void renderTipsPanel();
	void showWarningDialogue();
	void showFiberSettingPanel();
	bool enableSettingInput = true;
	bool enableBundling = false;
	bool showWarning = false;
	bool showFiberSetting = false;
	//UI callbacks

	//for statistics
	double fps = 0;
	int cnt = 0;
	int frameCount = 0;
	int currentTime = 0;
	int previousTime = 0;
	void showFps();
	double aveFps = 0;

};
