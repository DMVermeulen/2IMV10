#include"Application.h"
#include <stdio.h>
#include"imgui_internal.h"

Application* Application::m_app = nullptr;

Application::Application(){
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
	scene.initComputeShaders();
	scene.addInstance("models\\whole_brain.tck"); 
	scene.addInstance("models\\AF_left.tck");
	scene.setActivatedInstance(0);
	//scene.removeInstance(0);
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
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "FiberVisualizer", nullptr, nullptr);
	if (window == nullptr)
		std::runtime_error("Create window failed!");
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, window_size_update_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetWindowIconifyCallback(window, window_iconify_callback);
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

	fileDialog.SetTitle("title");
	fileDialog.SetTypeFilters({ ".tck" });
}

void Application::initRenderer() {
	renderer.setScene(&scene);
	renderer.setCamera(&camera);
	renderer.setViewportSize(SCR_WIDTH, SCR_HEIGHT);
	renderer.init();
	renderer.updateShadingPassInstanceInfo();
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
		//if (show_demo_window)
		//	ImGui::ShowDemoWindow(&show_demo_window);

		renderUI();

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);

		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		if(!suspendRender && !scene.isEmpty())
		  renderFrame();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}
}

void Application::renderUI() {
	//ImGuiStyle& style = ImGui::GetStyle();
	//ImVec4 bgColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); // white background color
	//ImVec4 textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // black text color
	//style.Colors[ImGuiCol_WindowBg] = bgColor;
	renderSettingPanel();
	//renderTipsPanel();
}

void Application::showWarningDialogue() {

	ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	ImVec2 windowSize(400, 200);
	ImVec2 windowPos((displaySize.x - windowSize.x) * 0.5f, (displaySize.y - windowSize.y) * 0.5f);
	ImGui::SetNextWindowPos(windowPos);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4 bgColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // white background color
	ImVec4 textColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); // black text color
	style.Colors[ImGuiCol_WindowBg] = bgColor;
	ImGui::PushStyleColor(ImGuiCol_Text, textColor);

	//ImGui::Begin("Warning",&showWarning);
	if (!ImGui::Begin("Warning", &showWarning, ImGuiWindowFlags_NoMove))
	{
		ImGui::End();
		return;
	}
	ImGui::Spacing();
	ImGui::Text("This functionality requires at least 2GB video memory. Are you sure to Continue?");
	ImGui::Spacing();

	if (ImGui::Button("Yes")) {
		showFiberSetting = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("No")) {

		showWarning = false;
		enableBundling = false;
		enableSettingInput = true;
	}

	ImGui::End();

	bgColor = ImVec4(0.0f, 0.0f, 0.0f, 0.85f); // white background color
	textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // black text color
	style.Colors[ImGuiCol_WindowBg] = bgColor;
	ImGui::PushStyleColor(ImGuiCol_Text, textColor);
}

void Application::showFiberSettingPanel() {

}

void Application::renderSettingPanel() {
		static float cameraSpeed = 80.0f;
		static float mouseSensitivity = 0.0f;
		static float tubeRadius = 0.1f;
		static float tubeGranularity = 1;
		static float bundlingAcc = 0.5f;
		static float requiredVideoMem = 2.7;
		static float fiberBundling = 0;
		const static float bundleScaleFactor = 30; //45
		static float lineWidth = 0.1f;
		static float roughness = 0.2f;
		static float metallic = 0.8f;
		static float ssao = 0.5f;
		static float contrast = 0.6f;
		static float colorInterval = 0;
		static float brightness = 0;
		static float saturation = 1;
		static float sharpening = 0;
		static int counter = 0;
		static glm::vec3 slicingPos;
		static glm::vec3 slicingDir;
		static bool enableSlicing = false;
		static int colorMode = 0;
		static int lightingMode = 0;
		static glm::vec3 colorMapConstant=glm::vec3(0,0,1);
		static glm::vec3 bgColor = glm::vec3(0, 0, 0);
		static std::vector<std::string> items = { "instance 0 ", "instance 1" };
		//static std::vector<std::string> items = { "instance 0 " };

		//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		//ImGui::End();

		ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 350, 0));

		ImGui::Begin("Settings");
		if (ImGui::CollapsingHeader("Models"))
		{
			static int currentItemInstance = 0;
			// Add new instance
			if (ImGui::Button("Add..."))
					fileDialog.Open();
			fileDialog.Display();
			if (fileDialog.HasSelected())
			{
				std::string fileName= fileDialog.GetSelected().string();
				std::size_t pos = fileName.find_last_of('\\'); // Find the last '\' character
				std::string instanceName;
				if (pos != std::string::npos) 
				   instanceName = fileName.substr(pos + 1);
				items.push_back(instanceName);
				scene.addInstance(fileName);
				scene.setActivatedInstance(int(items.size()) - 1);
				scene.getInstanceSettings(&fiberBundling, &enableSlicing, &slicingPos, &slicingDir);
				currentItemInstance = int(items.size()) - 1;
				fileDialog.ClearSelected();
			}
			ImGui::SameLine();
			// Remove instance
			if (ImGui::Button("Remove")) {
				scene.removeInstance(currentItemInstance);
				items.erase(items.begin() + currentItemInstance);
				if (items.size() > 0) {
					scene.setActivatedInstance(0);
					scene.getInstanceSettings(&fiberBundling, &enableSlicing, &slicingPos, &slicingDir);
					fiberBundling *= bundleScaleFactor;
					renderer.updateShadingPassInstanceInfo();
					currentItemInstance = 0;
				}
				else {
					currentItemInstance = -1;
					enableBundling = false;
					fiberBundling = 0;
					scene.updateFiberBundlingStatus(enableBundling);
				}
			}
			
			//Select an instance to visualize
			if (items.size() > 0) {
				if (ImGui::BeginCombo("##instances", items[currentItemInstance].c_str())) {
					for (int i = 0; i < items.size(); i++) {
						bool isSelected = (currentItemInstance == i);
						if (ImGui::Selectable(items[i].c_str(), isSelected)) {
							currentItemInstance = i;
							scene.setActivatedInstance(i);
							scene.getInstanceSettings(&fiberBundling, &enableSlicing, &slicingPos, &slicingDir);
							fiberBundling *= bundleScaleFactor;
							renderer.updateShadingPassInstanceInfo();
						}
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}
		}

		if (ImGui::CollapsingHeader("View"))
		{
			if (ImGui::SliderFloat("camera speed", &cameraSpeed, 0.0f, 200.0f)) {
				camera.setSpeed(cameraSpeed);
			}
		}

		if (ImGui::CollapsingHeader("Fiber Bundling"))
		{
			if (ImGui::Checkbox("Enable", &enableBundling)) {
				 scene.updateFiberBundlingStatus(enableBundling);
				 if (false == enableBundling)
					 fiberBundling = 0;
				 //if (enableBundling) {
					// showWarning = true;
					// enableSettingInput = false;
				 //}
			}

			if (enableBundling) {
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0, 0, 1.0f));
				ImFont* font = ImGui::GetIO().Fonts->Fonts[0]; 
				float fontSize = 35.0f;
				font->Scale = fontSize / font->FontSize;
				ImGui::Text("        Required video memory: %.1f GB", scene.getRequiredVideoMem(bundlingAcc));
				font->Scale = 1.0f;
				ImGui::PopStyleColor();

				if (ImGui::SliderFloat("bundling", &fiberBundling, 0.0f, 1.0f)) {
					scene.edgeBundling(fiberBundling/ bundleScaleFactor);
				}
				if (ImGui::SliderFloat("accuracy", &bundlingAcc, 0.0f, 1.0f)) {
					scene.setBundlerAccuracy(bundlingAcc);
				}
			}

			if (ImGui::SliderFloat("Line width", &lineWidth, 0.0f, 1.0f)) {
				renderer.setLineWidth(lineWidth*3);
			}

		}

		if (ImGui::CollapsingHeader("Slicing"))
		{
			if (ImGui::Checkbox("Enable Slicing", &enableSlicing) && enableSettingInput) {
				scene.updateInstanceEnableSlicing(slicingPos,slicingDir,enableSlicing);
			}

			//if (ImGui::SliderFloat3("slicing pos", &slicingPos.x, 0, 1.0f)) {
			//	scene.slicing(slicingPos, slicingDir);
			//}

			//if (ImGui::SliderFloat3("slicing dir", &slicingDir.x, 0, 1.0f)) {
			//	scene.slicing(slicingPos, slicingDir);
			//}

			ImGui::LabelText("##label SlicingCenter", "Center");
			ImGui::BeginGroup(); 
			ImGui::PushItemWidth(70);
			if (ImGui::SliderFloat("##CenterX", &slicingPos.x, 0, 1.0f, "X: %.2f")) {
				scene.slicing(slicingPos, slicingDir);
			}
			ImGui::PopItemWidth();

			ImGui::SameLine(); 

			ImGui::PushItemWidth(70);
			if (ImGui::SliderFloat("##CenterY", &slicingPos.y, 0, 1.0f, "Y: %.2f")) {
				scene.slicing(slicingPos, slicingDir);
			}
			ImGui::PopItemWidth();

			ImGui::SameLine();

			ImGui::PushItemWidth(70);
			if (ImGui::SliderFloat("##CenterZ", &slicingPos.z, 0, 1.0f, "Z: %.2f")) {
				scene.slicing(slicingPos, slicingDir);
			}
			ImGui::PopItemWidth();

			ImGui::EndGroup(); 


			ImGui::LabelText("##label SlicingDir", "Direction");
			ImGui::BeginGroup();
			ImGui::PushItemWidth(70);
			if (ImGui::SliderFloat("##DirX", &slicingDir.x, 0, 1.0f, "X: %.2f")) {
				scene.slicing(slicingPos, slicingDir);
			}
			ImGui::PopItemWidth();

			ImGui::SameLine();

			ImGui::PushItemWidth(70);
			if (ImGui::SliderFloat("##DirY", &slicingDir.y, 0, 1.0f, "Y: %.2f")) {
				scene.slicing(slicingPos, slicingDir);
			}
			ImGui::PopItemWidth();

			ImGui::SameLine();

			ImGui::PushItemWidth(70);
			if (ImGui::SliderFloat("##DirZ", &slicingDir.z, 0, 1.0f, "Z: %.2f")) {
				scene.slicing(slicingPos, slicingDir);
			}
			ImGui::PopItemWidth();

			ImGui::EndGroup();

		}

		//if (ImGui::CollapsingHeader("Materials"))
		//{
		//	if (ImGui::SliderFloat("roughness", &roughness, 0.0f, 1.0f)) {
		//		scene.setInstanceMaterial(roughness, metallic);
		//	}
		//	if (ImGui::SliderFloat("metallic", &metallic, 0.0f, 1.0f)) {
		//		scene.setInstanceMaterial(roughness, metallic);
		//	}
		//}

		if (ImGui::CollapsingHeader("Background"))
		{
			if (ImGui::ColorPicker3("BgColor", (float*)&bgColor)) {
				renderer.setBgColor(bgColor);
			}
		}

		if (ImGui::CollapsingHeader("Colormap"))
		{
			static const char* colorModes[] = { "direction ", "normal","constant" };
			static int currentItemColorMap = 0;
			if (ImGui::BeginCombo("##colorMapping", colorModes[currentItemColorMap])) {
				for (int i = 0; i < IM_ARRAYSIZE(colorModes); i++) {
					bool isSelected = (currentItemColorMap == i);
					if (ImGui::Selectable(colorModes[i], isSelected)) {
						currentItemColorMap = i;
						renderer.setColorMode(i);
					}
					if (isSelected && i != 2) 
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			if (2== currentItemColorMap) {
				if (ImGui::ColorPicker3("Color", (float*)&colorMapConstant, ImGuiColorEditFlags_NoInputs)) {
					renderer.setColorConstant(colorMapConstant);
				}
			}
		}

		if (ImGui::CollapsingHeader("Lighting mode"))
		{
			static const char* lightingModes[] = { "normal", "PBR" };
			static int currentItemLightingMode = 0;
			if (ImGui::BeginCombo("##lightingMode", lightingModes[currentItemLightingMode])) {
				for (int i = 0; i < IM_ARRAYSIZE(lightingModes); i++) {
					bool isSelected = (currentItemLightingMode == i);
					if (ImGui::Selectable(lightingModes[i], isSelected)) {
						currentItemLightingMode = i;
						renderer.setLightingMode(i);
					}
					if (isSelected && i != 1)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			if (1 == currentItemLightingMode) {
				if (ImGui::SliderFloat("roughness", &roughness, 0.0f, 1.0f)) {
					scene.setInstanceMaterial(roughness, metallic);
				}
				if (ImGui::SliderFloat("metallic", &metallic, 0.0f, 1.0f)) {
					scene.setInstanceMaterial(roughness, metallic);
				}
			}
		}

		if (ImGui::CollapsingHeader("Post effects"))
		{
			ImGui::PushItemWidth(210);
			if (ImGui::SliderFloat("Ambient occlusion", &ssao, 0.0f, 1.0f)) {
				renderer.setSSAORadius(ssao*50);
			}
			ImGui::PopItemWidth();

			ImGui::PushItemWidth(210);
			if (ImGui::SliderFloat("Contrast", &contrast, 0.0f, 1.0f)) {
				renderer.setContrast(contrast*2);
			}
			ImGui::PopItemWidth();

			ImGui::PushItemWidth(210);
			if (ImGui::SliderFloat("Brightness", &brightness, 0.0f, 1.0f)) {
				renderer.setBrightness(brightness);
			}
			ImGui::PopItemWidth();

			ImGui::PushItemWidth(210);
			if (ImGui::SliderFloat("Saturation", &saturation, 0.0f, 1.0f)) {
				renderer.setSaturation(saturation);
			}
			ImGui::PopItemWidth();

			ImGui::PushItemWidth(210);
			if (ImGui::SliderFloat("Sharpening", &sharpening, 0.0f, 1.0f)) {
				renderer.setSharpening(sharpening);
			}
			ImGui::PopItemWidth();

			ImGui::PushItemWidth(210);
			if (ImGui::SliderFloat("Color flattening", &colorInterval, 0.0f, 1.0f)) {
	            renderer.setColorFlattening(colorInterval/2);
            }
			ImGui::PopItemWidth();
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

	//some other windows
	if (showWarning)
		showWarningDialogue();
	if (showFiberSetting)
		showFiberSettingPanel();
}

void Application::renderTipsPanel() {
	ImGui::SetNextWindowSize(ImVec2(540, -1));
	//ImGui::SetNextWindowSizeConstraints(ImVec2(300, -1), ImVec2(FLT_MAX, FLT_MAX));
	ImGui::Begin("Tips");

	if (ImGui::CollapsingHeader("Models"))
	{
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Add");
		ImGui::Text("Add a model through the file browser.\nAutomatically switch to the new model");

		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Remove");
		ImGui::Text("Remove current model. \nAutomatically switch to the first one in the model list");
	}

	if (ImGui::CollapsingHeader("View"))
	{
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Camera speed");
		ImGui::Text("Speed of camera movement");
	}

	if (ImGui::CollapsingHeader("Fiber bundling"))
	{
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Enable");
		ImGui::Text("Enable fiber bundling. \nLarge textures on GPU will be created if enabled. \nMake sure you have at least 3GB video memory available");

		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "bundling");
		ImGui::Text("Control the degree of bundling. \nLarge value produces a more simplified model");

		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "lineWidth");
		ImGui::Text("As its named indicated");
	}

	if (ImGui::CollapsingHeader("Slicing"))
	{
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Enable");
		ImGui::Text("Slicing the model at arbitrary plane. \nThis does not require any additional memory, so feel free to enable it.");

		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Center");
		ImGui::Text("Anchor point of the slicing plane. \nValues are defined relative to the size of the bounding box of the model.");

		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Direction");
		ImGui::Text("Normal vector of the slicing plane. \nValues are defined relative to the size of the bounding box of the model.");
	}

	if (ImGui::CollapsingHeader("Color mapping"))
	{
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "option: direction");
		ImGui::Text("Shade a fragment by its direction vector");

		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "option: normal");
		ImGui::Text("Shade a fragment by its normal vector (changes with your camera view)");

		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "option: constant");
		ImGui::Text("Assgin a constant color.");
	}

	if (ImGui::CollapsingHeader("Lighting mode"))
	{
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "option: normal");
		ImGui::Text("No lighting applied");

		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "option: PBR");
		ImGui::Text("Physical based rendering. \nEnable defining roughness and metalness");
	}

	if (ImGui::CollapsingHeader("Post effects"))
	{
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Ambient occlusion");
		ImGui::Text("Control the area of shadow produced.");

		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Contrast");
		ImGui::Text("as its name indicated");

		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Brightness");
		ImGui::Text("as its name indicated");

		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Saturation");
		ImGui::Text("as its name indicated");

		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Sharpening");
		ImGui::Text("as its name indicated");

		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Color flattening");
		ImGui::Text("Distritize the color space. \nProduce toon-like style (not good yet)");
	}
	ImGui::End();


	//if (ImGui::CollapsingHeader("Colormap"))
	//{
	//	static const char* colorModes[] = { "direction ", "normal","constant" };
	//	static int currentItemColorMap = 0;
	//	if (ImGui::BeginCombo("##colorMapping", colorModes[currentItemColorMap])) {
	//		for (int i = 0; i < IM_ARRAYSIZE(colorModes); i++) {
	//			bool isSelected = (currentItemColorMap == i);
	//			if (ImGui::Selectable(colorModes[i], isSelected)) {
	//				currentItemColorMap = i;
	//				renderer.setColorMode(i);
	//			}
	//			if (isSelected && i != 2)
	//				ImGui::SetItemDefaultFocus();
	//		}
	//		ImGui::EndCombo();
	//	}
	//	if (2 == currentItemColorMap) {
	//		if (ImGui::ColorPicker3("Color", (float*)&colorMapConstant, ImGuiColorEditFlags_NoInputs)) {
	//			renderer.setColorConstant(colorMapConstant);
	//		}
	//	}
	//}


}

void Application::renderFrame() {
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	renderer.renderFrame();
	//glfwSwapBuffers(window);
	//showFps();
}

//handle window resizing
void Application::window_size_update_callback(GLFWwindow* window, int width, int height) {
	m_app->SCR_WIDTH = width;
	m_app->SCR_HEIGHT = height;
	glViewport(0, 0, width, height);

	//update the renderer
	if(width>0 && height>0)
	 m_app->renderer.updateViewportSize(width, height);

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

void Application::window_iconify_callback(GLFWwindow* window, int iconified) {
	if (iconified) {
		// window was minimized
		m_app->suspendRender = true;
	}
	else {
		// window was restored from minimized state
		m_app->suspendRender = false;
	}
}

void Application::showFps() {
	if (cnt > 200)
		return;

	frameCount++;
	currentTime = glfwGetTime();

	// Calculate time difference
	double timeInterval = currentTime - previousTime;

	if (timeInterval > 0.5) {
		fps = frameCount / timeInterval;
		previousTime = currentTime;
		frameCount = 0;
		//std::cout << "FPS: " << fps << std::endl;
		aveFps = (aveFps * cnt + fps) / (cnt + 1);
		//std::cout << "ave FPS: " << aveFps << std::endl;
		cnt++;
	}
}



