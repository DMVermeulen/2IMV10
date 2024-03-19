#include"Application.h"
#include <stdio.h>

Application* Application::m_app = nullptr;

Application::Application(Scene& _scene):scene(_scene){
	m_app = this;
	float lastX = (float)SCR_WIDTH / 2.0;
	float lastY = (float)SCR_HEIGHT / 2.0;
}

Application::~Application() {
	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::run() {
	initWindow();
	initScene();
	initRenderer();
	mainLoop();
}

void Application::initScene() {
	scene.addInstance("models\\whole_brain.tck"); 
	scene.addInstance("models\\AF_left.tck");
	scene.setActivatedInstance(0);

	//test
	//scene.updateInstanceEnableSlicing();
	scene.slicing(glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
}


static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void Application::initWindow() {
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		std::runtime_error("Init glfw failed!");

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	const char* glsl_version = "#version 100";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
	// GL 3.2 + GLSL 150
	const char* glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

	// Create window with graphics context
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
	if (window == nullptr)
		std::runtime_error("Create window failed!");
	glfwMakeContextCurrent(window);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	gladLoadGL();
	glfwSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
	ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
#endif
	ImGui_ImplOpenGL3_Init(glsl_version);
}

void Application::initRenderer() {
	renderer.init();
	renderer.setScene(&scene);
	renderer.updateShadingPassInstanceInfo();
	renderer.setCamera(&camera);
	renderer.setViewportSize(SCR_WIDTH, SCR_HEIGHT);
}

void Application::mainLoop() {
	while (!glfwWindowShouldClose(window))
	{
		io = ImGui::GetIO();
		ImGuiIOWantCaptureMouse = io.WantCaptureMouse;
		ImGuiIOWantCaptureKeyboard = io.WantCaptureKeyboard;

		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);

		glfwPollEvents();
		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		renderUI();

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);

		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		renderFrame();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}
}

void Application::renderUI() {
		static float cameraSpeed = 0.0f;
		static float mouseSensitivity = 0.0f;
		static float tubeRadius = 0.1f;
		static float tubeGranularity = 1;
		static float fiberBundling = 0;
		static float lineWidth = 0.1f;
		static float roughness = 0.2f;
		static float metallic = 0.8f;
		static float ssao = 0.2f;
		static float colorInterval = 0;
		static float contrast = 0.5;
		static int counter = 0;
		static glm::vec3 slicingPos;
		static glm::vec3 slicingDir;
		static bool enableSlicing = false;

		//ImGui::Begin("Settings");                          // Create a window called "Hello, world!" and append into it.

		//ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		//ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		//ImGui::Checkbox("Another Window", &show_another_window);


		////if (ImGui::SliderFloat("tube size", &tubeRadius, 0.0f, 10.0f)) {
		////	scene.setRadius(tubeRadius);
		////}
		////if (ImGui::SliderFloat("tube granularity", &tubeGranularity, 0.0f, 1.0f)) {
		////	scene.setNTris(int(8 * tubeGranularity));
		////}

		//if (ImGui::SliderFloat("SSAO", &ssao, 0.0f, 1.0f)) {
		//	renderer.setSSAORadius(ssao*50);
		//}
		//if (ImGui::SliderFloat("Contrast", &contrast, 0.0f, 1.0f)) {
		//	renderer.setContrast(contrast*2);
		//}
		//if (ImGui::SliderFloat("Color flattening", &colorInterval, 0.0f, 1.0f)) {
		//	renderer.setColorFlattening(colorInterval/2);
		//}
		//ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		//if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
		//	counter++;
		//ImGui::SameLine();
		//ImGui::Text("counter = %d", counter);

		//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		//ImGui::End();


		ImGui::Begin("Settings");
		if (ImGui::CollapsingHeader("Models"))
		{
			if (ImGui::SliderFloat("Color flattening", &colorInterval, 0.0f, 1.0f)) {
				renderer.setColorFlattening(colorInterval / 2);
			}
			//Select an instance to visualize
			static const char* items[] = { "instance 0 ", "instance 1" };
			static int currentItem = 0;
			if (ImGui::BeginCombo("##combo", items[currentItem])) {
				for (int i = 0; i < IM_ARRAYSIZE(items); i++) {
					bool isSelected = (currentItem == i);
					if (ImGui::Selectable(items[i], isSelected)) {
						currentItem = i;
						scene.setActivatedInstance(i);
						//getInstanceSettings();
						renderer.updateShadingPassInstanceInfo();
					}
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}

		if (ImGui::CollapsingHeader("View"))
		{
			if (ImGui::SliderFloat("camera speed", &cameraSpeed, 0.0f, 200.0f)) {
				camera.setSpeed(cameraSpeed);
			}
			if (ImGui::SliderFloat("mouse sensitivity", &mouseSensitivity, 0.0f, 1.0f)) {
				camera.setSpeed(mouseSensitivity);
			}
		}

		if (ImGui::CollapsingHeader("Bundling"))
		{
			if (ImGui::SliderFloat("Line width", &lineWidth, 0.0f, 1.0f)) {
				renderer.setLineWidth(lineWidth*3);
			}

			if (ImGui::SliderFloat("bundling", &fiberBundling, 0.0f, 1.0f)) {
				scene.edgeBundling(fiberBundling/25,tubeRadius, int(8 * tubeGranularity));
			}
		}

		if (ImGui::CollapsingHeader("Slicing"))
		{
			if (ImGui::Checkbox("Enable", &enableSlicing)) {
				scene.updateInstanceEnableSlicing(slicingPos,slicingDir);
			}

			if (ImGui::SliderFloat3("slicing pos", &slicingPos.x, 0, 1.0f)) {
				scene.slicing(slicingPos, slicingDir);
			}

			if (ImGui::SliderFloat3("slicing dir", &slicingDir.x, 0, 1.0f)) {
				scene.slicing(slicingPos, slicingDir);
			}
		}

		if (ImGui::CollapsingHeader("Materials"))
		{
			if (ImGui::SliderFloat("roughness", &roughness, 0.0f, 1.0f)) {
				scene.setInstanceMaterial(roughness, metallic);
			}
			if (ImGui::SliderFloat("metallic", &metallic, 0.0f, 1.0f)) {
				scene.setInstanceMaterial(roughness, metallic);
			}
		}



		ImGui::End();

	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}
}

void Application::renderFrame() {
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	renderer.renderFrame();
	//glfwSwapBuffers(window);
}

void Application::mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	if (m_app->ImGuiIOWantCaptureMouse || m_app->ImGuiIOWantCaptureKeyboard)
		return;

	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (m_app->firstMouse)
	{
		m_app->lastX = xpos;
		m_app->lastY = ypos;
		m_app->firstMouse = false;
	}

	float xoffset = xpos - m_app->lastX;
	float yoffset = m_app->lastY - ypos; // reversed since y-coordinates go from bottom to top

	m_app->lastX = xpos;
	m_app->lastY = ypos;
	
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		m_app->camera.ProcessMouseMovement(xoffset, yoffset);
}

void Application::processInput(GLFWwindow* window) {
	if (m_app->ImGuiIOWantCaptureMouse || m_app->ImGuiIOWantCaptureKeyboard)
		return;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

void Application::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	m_app->camera.ProcessMouseScroll(static_cast<float>(yoffset));
}


