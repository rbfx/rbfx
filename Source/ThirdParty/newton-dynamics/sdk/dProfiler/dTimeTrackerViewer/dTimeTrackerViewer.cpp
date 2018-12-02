// dTimeTrackerViewer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "dProfilerTrace.h"
#include "imgui_impl_glfw.h"
#include "dTimeTrackerViewer.h"


int _tmain(int argc, _TCHAR* argv[])
{
	dTimeTrackerViewer app;
	app.Run();
}


dTimeTrackerViewer::dTimeTrackerViewer()
	:m_recentFiles()
	,m_currentTrace(NULL)
{
	// Setup window
	glfwSetErrorCallback(error_callback);
//	if (!glfwInit()) {
//		return 1;
	glfwInit();

	m_window = glfwCreateWindow(1280, 720, "Time Tracker visualizer", NULL, NULL);
	glfwMakeContextCurrent(m_window);

	// Setup ImGui binding
	ImGui_ImplGlfw_Init(m_window, true);

	ImGuiIO& io = ImGui::GetIO();
	io.UserData = this;

//	io.Fonts->AddFontDefault();

	bool show_test_window = true;
	bool show_another_window = false;
	m_clear_color = ImColor(255, 255, 255);
	m_keyboardChainCallback = glfwSetKeyCallback(m_window, KeyCallback);

	// 
	m_recentFiles.Append("../xxxx.tt");
	m_recentFiles.Append("../xxxx1.tt");
}

dTimeTrackerViewer::~dTimeTrackerViewer()
{
	if (m_currentTrace) {
		delete m_currentTrace;
	}
	// Cleanup
	ImGui_ImplGlfw_Shutdown();
	glfwTerminate();
}

void dTimeTrackerViewer::error_callback(int error, const char* description)
{
	fprintf(stderr, "Error %d: %s\n", error, description);
}

void dTimeTrackerViewer::KeyCallback(GLFWwindow* const window, int key, int, int action, int mods)
{
	ImGuiIO& io = ImGui::GetIO();
	dTimeTrackerViewer* const me = (dTimeTrackerViewer*) io.UserData;
	me->m_keyboardChainCallback(window, key, 0, action, mods);
	if (key == GLFW_KEY_ESCAPE) {
		glfwSetWindowShouldClose(window, 1);
	}
}

void dTimeTrackerViewer::Run()
{
	// Main loop
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
		ImGui_ImplGlfw_NewFrame();

		ImGuiIO& io = ImGui::GetIO();
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Open", "")) {
					dAssert(0);
				}
				if (m_recentFiles.GetCount()) {
					ImGui::Separator();
					for (dList<dString>::dListNode* node = m_recentFiles.GetFirst(); node; node = node->GetNext()) {
						const dString& name = node->GetInfo();
						if (ImGui::MenuItem(name.GetStr(), "")) {
							FILE* const file = fopen (name.GetStr(), "rb");
							if (file) {
								if (m_currentTrace) {
									delete m_currentTrace;
								}
								m_currentTrace = new dProfilerTrace(file);
								fclose (file);
							}
						}
					}
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help")) {
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		if (m_currentTrace) {
			bool openflag = true;
			ImGuiWindowFlags window_flags = 0;
			window_flags |= ImGuiWindowFlags_NoTitleBar;
			window_flags |= ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoResize;
			window_flags |= ImGuiWindowFlags_NoSavedSettings;

			//ImVec2 cursorPosit(ImGui::GetCursorScreenPos());
			//ImVec2 windowsPosit(ImGui::GetCursorPos());
			//ImVec2 windowsSize(ImGui::GetWindowSize());
			ImVec2 windowsPosit(io.DisplayVisibleMin);
			ImVec2 windowsSize(io.DisplaySize);
			windowsPosit.y += 20.0f;
			windowsSize.y -= 20.0f;

			ImGui::SetNextWindowPos(windowsPosit, ImGuiSetCond_Always);
			ImGui::SetNextWindowSize(windowsSize, ImGuiSetCond_Always);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

			ImGui::Begin("", &openflag, window_flags);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
				m_currentTrace->Render(this);
				ImGui::PopStyleColor();
			ImGui::End();

			ImGui::PopStyleColor();
		}

		// Rendering
		int display_w, display_h;
		glfwGetFramebufferSize(m_window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(m_clear_color.x, m_clear_color.y, m_clear_color.z, m_clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		glfwSwapBuffers(m_window);
	}
}