#include <iostream>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>
#include <GL/glew.h>
#include <SDL.h>
#include "Frame.hpp"

#include <vector>

struct Keyframe {
    float time;
    float value;
    bool selected = false;
};

std::vector<Keyframe> keyframes;
float timeline_min = 0.0f, timeline_max = 10.0f;
float current_time = 0.0f;
float zoom = 100.0f;  // pixels per unit time
bool playing = false;
float play_speed = 1.0f;


int main(int argc, char** argv) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		std::cout << "Error: " << SDL_GetError() << std::endl;
		return 1;
	}
	const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("ImGui SDL2+OpenGL3 Image Display", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW init error" << std::endl;
        return -1;
    }

    GLuint image_texture = 0;
	std::shared_ptr<CCTV::Frame> test = CCTV::Frame::FromFile("../contrib/test/dynamic/1.png");	
	std::shared_ptr<CCTV::IGLTexture> texture = test->GetTexture();
    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Image Viewer");
        if (texture != nullptr) {
            ImGui::Text("Image size: %d x %d", test->GetWidth(), test->GetHeight());
            ImGui::Image((ImTextureID)(intptr_t)texture->GetTexture(), ImVec2((float)test->GetWidth(), (float)test->GetHeight()));
        } else {
            ImGui::Text("No image loaded");
        }
        ImGui::End();

		ImGui::Begin("Timeline & Image Viewer");

		ImGui::SliderFloat("Zoom", &zoom, 20.0f, 500.0f);
		ImGui::SliderFloat("Min Time", &timeline_min, -5.0f, 5.0f);
		ImGui::SliderFloat("Max Time", &timeline_max, 5.0f, 15.0f);
		if (ImGui::Button(playing ? "Pause" : "Play")) playing = !playing;
		ImGui::SameLine(); ImGui::SliderFloat("Speed", &play_speed, 0.1f, 5.0f);

		static float last_time = 0.0f;
		float now = (float)SDL_GetTicks() / 1000.0f;
		if (playing) {
			current_time += (now - last_time) * play_speed;
			if (current_time > timeline_max) current_time = timeline_min;
		}
		last_time = now;
		ImGui::SliderFloat("Current Time", &current_time, timeline_min, timeline_max);

		float anim_value = 0.0f;
		keyframes = {
    		{0.0f, 0.0f}, {2.0f, 1.0f}, {4.0f, -1.0f}, {6.0f, 0.5f}, {8.0f, 0.0f}, {10.0f, 1.0f}
		};
		for (size_t i = 0; i < keyframes.size(); ++i) {
			if (current_time >= keyframes[i].time) {
				anim_value = keyframes[i].value;
				if (i + 1 < keyframes.size() && current_time < keyframes[i + 1].time) {
					float t = (current_time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
					anim_value = keyframes[i].value + t * (keyframes[i + 1].value - keyframes[i].value);
				}
			}
		}

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
		ImVec2 canvas_sz(600, 100);
		ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);
		draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
		draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

		int tick_count = 10;
		for (int i = 0; i <= tick_count; ++i) {
			float t = timeline_min + (timeline_max - timeline_min) * i / tick_count;
			ImVec2 tick_pos(canvas_p0.x + (t - timeline_min) / (timeline_max - timeline_min) * canvas_sz.x, canvas_p1.y);
			draw_list->AddLine(ImVec2(tick_pos.x, canvas_p0.y), tick_pos, IM_COL32(255, 255, 255, 100));
			//char buf[32]; ImFormatString(buf, 32, "%.1f", t);
			draw_list->AddText(ImVec2(tick_pos.x - 20, canvas_p1.y + 5), IM_COL32(255, 255, 255, 255), "biba");
		}

		for (auto& kf : keyframes) {
			float x = canvas_p0.x + (kf.time - timeline_min) / (timeline_max - timeline_min) * canvas_sz.x;
			ImVec2 kf_center(x, canvas_p0.y + canvas_sz.y * 0.5f);
			//ImVec2 kf_pos = kf_center - ImVec2(5, 5);
			ImU32 col = kf.selected ? IM_COL32(255, 255, 0, 255) : IM_COL32(0, 255, 0, 255);
			draw_list->AddCircleFilled(kf_center, 5.0f, col);
			if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) kf.selected = true;
		}

		ImVec2 scrubber_top(canvas_p0.x + (current_time - timeline_min) / (timeline_max - timeline_min) * canvas_sz.x, canvas_p0.y);
		ImVec2 scrubber_bot = ImVec2(scrubber_top.x, canvas_p1.y);
		draw_list->AddLine(scrubber_top, scrubber_bot, IM_COL32(255, 0, 0, 255), 2.0f);

		ImGui::Dummy(canvas_sz);
		ImGui::End();

        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    glDeleteTextures(1, &image_texture);
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}