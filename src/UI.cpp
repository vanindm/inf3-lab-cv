#include <algorithm>
#include <iostream>
#include <string>

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl2.h>
#include <imgui.h>

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_surface.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
}

#include <portable-file-dialogs.h>

#include "Frame.hpp"
#include <PATypes/Sequence.h>

static std::string currentError;
static bool errorPopupOpen = 0;

static CCTV::FrameSequence OpenFrameSequence() {
    try {
        std::vector<std::string> result =
            pfd::open_file("Открыть видеофайл", "", {"*"}).result();
        if (result.size() > 0)
            return CCTV::FrameSequence::LoadFromVideo(result[0], 0);
        else
            throw std::invalid_argument("Пользователь не выбрал файл");
    } catch (const std::invalid_argument &e) {
        throw e;
    } catch (const std::exception &e) {
        throw std::runtime_error(e.what());
    }
}

static void DisplayEvents(CCTV::FrameSequence &frames, int &currentIndex) {
    ImGui::BeginChild("EventList");
    if (frames.GetTagCount() > 0) {
        auto enumerator = frames.GetTagEnumerator();
        while (enumerator->moveNext()) {
            CCTV::ITag* ptr = enumerator->current().getSecond();
            if (ptr != nullptr) {
                std::string label = ptr->GetName() + " на кадре " + std::to_string(enumerator->current().getFirst());
                if (ImGui::Selectable(label.c_str())) {
                    currentIndex = enumerator->current().getFirst();
                }
            }
        }
        delete enumerator;
    } else {
        ImGui::Text("Событий не произошло");
    }
    ImGui::EndChild();
}

static void DrawFrameSequenceTimeline(const char *id,
                                      CCTV::FrameSequence &frames,
                                      int &currentIndex, bool &playing,
                                      float &fps, float &zoomPxPerFrame) {
    const int n = frames.getLength();
    if (n <= 0) {
        ImGui::TextUnformatted("Последовательность кадров пуста.");
        return;
    }

    float treshold = frames.GetTreshold();
    float leapTreshold = frames.GetLeapTreshold();

    currentIndex = std::clamp(currentIndex, 0, n - 1);

    if (ImGui::Button(playing ? "||" : "|>") || ImGui::Shortcut(ImGuiKey_Space))
        playing = !playing;

    ImGui::SameLine();
    if (ImGui::Button("<<"))
        currentIndex = 0;

    ImGui::SameLine();
    if (ImGui::Button("<") || ImGui::Shortcut(ImGuiKey_LeftArrow))
        currentIndex = std::max(0, currentIndex - 1);

    ImGui::SameLine();
    if (ImGui::Button(">") || ImGui::Shortcut(ImGuiKey_RightArrow))
        currentIndex = std::min(n - 1, currentIndex + 1);

    ImGui::SameLine();
    if (ImGui::Button(">>"))
        currentIndex = n - 1;

    int windowLength = frames.GetWindow();
    ImGui::SliderInt("Кадр", &currentIndex, 0, n - 1);
    ImGui::SliderFloat("К/с", &fps, 1.0f, 120.0f, "%.1f");
    ImGui::SliderInt("Размер окна", &windowLength, 0, frames.getLength());
    ImGui::SliderFloat("Порог значимости", &treshold, 0, 2000.f, "%.1f");
    ImGui::SliderFloat("Порог скачка", &leapTreshold, 0, 1000.f, "%.1f");
    frames.SetWindow(windowLength);
    frames.SetFramerate(fps);
    frames.SetTreshold(treshold);
    frames.SetLeapTreshold(leapTreshold);
    if (ImGui::Button("Предпосчитать")) {
        frames.PrecalcScore();
    }

    static float playAccum = 0.0f;
    if (playing) {
        playAccum += ImGui::GetIO().DeltaTime * (fps);
        while (playAccum >= 1.0f) {
            playAccum -= 1.0f;
            (currentIndex)++;
            if (currentIndex >= n)
                currentIndex = 0;
        }
    } else {
        playAccum = 0.0f;
    }

    const float rowH = 28.0f;
    const float canvasH = 60.0f;
    const float w = std::max(1.0f, zoomPxPerFrame);
    const float totalW = n * w;

    ImGui::Text("Кадры: %d", n);

    ImGui::BeginChild(id, ImVec2(0, canvasH), ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_HorizontalScrollbar);

    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImDrawList *dl = ImGui::GetWindowDrawList();

    ImGui::Dummy(ImVec2(totalW, rowH - 4.0f));
    const bool hovered = ImGui::IsItemHovered();

    const float scrollX = ImGui::GetScrollX();
    const float baseX = p0.x - scrollX;
    const float baseY = p0.y;

    for (int i = 0; i < n; ++i) {
        const float x0 = baseX + i * w;
        const float x1 = x0 + w - 1.0f;

        ImU32 col = IM_COL32((60 + 160), (60 + 160), (60 + 160), 255);
        ImU32 border = IM_COL32(60, 60, 60, 255);

        if (i == currentIndex) {
            col = IM_COL32(90, 140, 220, 255);
            border = IM_COL32(220, 220, 220, 255);
        }

        dl->AddRectFilled(ImVec2(x0, baseY + 6.0f),
                          ImVec2(x1, baseY + 6.0f + rowH), col);
        dl->AddRect(ImVec2(x0, baseY + 6.0f), ImVec2(x1, baseY + 6.0f + rowH),
                    border);
    }

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const float mx = ImGui::GetIO().MousePos.x - baseX;
        const int idx = (int)std::floor(mx / w);
        if (0 <= idx && idx < n)
            currentIndex = idx;
    }

    if (hovered) {
        const float mx = ImGui::GetIO().MousePos.x - baseX;
        const int idx = (int)std::floor(mx / w);
        if (frames.GetWindow() <= idx && idx < n) {
            float score = frames.GetScore(idx);
            ImGui::BeginTooltip();
            ImGui::Text("Кадр: %d", idx);
            ImGui::Text("Важность : %.3f", score);
            ImGui::EndTooltip();
        }
    }

    ImGui::EndChild();
    ImGui::SliderFloat("Масштаб (пикс./кадр)", &zoomPxPerFrame, 2.0f, 40.0f,
                       "%.1f", ImGuiSliderFlags_Logarithmic);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cout << "Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    const char *glsl_version = "#version 130";

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                          SDL_WINDOW_ALLOW_HIGHDPI);

    SDL_Window *window =
        SDL_CreateWindow("Анализ видеофайла", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, 1000, 700, window_flags);

    SDL_Surface *favicon = IMG_Load("../contrib/favicon.png");
    SDL_SetWindowIcon(window, favicon);
    SDL_FreeSurface(favicon);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW init error" << std::endl;
        return -1;
    }

    CCTV::FrameSequence frames(30);

    int currentIndex = 0;
    bool playing = false;
    float fps = frames.GetFramerate();
    float zoom = 10.0f;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    try {
        io.Fonts->AddFontFromFileTTF("../contrib/cour.ttf", 14.0f, NULL,
                                     io.Fonts->GetGlyphRangesCyrillic());
    } catch (std::exception &e) {
        std::cout << "Error: " << e.what() << std::endl;
    }

    ImVec4 clearColor = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);

    bool done = false;

    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if (errorPopupOpen) {
            ImGui::OpenPopup("Ошибка");
        }

        if (ImGui::BeginPopupModal("Ошибка", &errorPopupOpen,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", currentError.c_str());
            if (ImGui::Button("Закрыть", ImVec2(60, 0))) {
                ImGui::CloseCurrentPopup();
                errorPopupOpen = 0;
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Файл")) {
                if (ImGui::MenuItem("Открыть...", "Ctrl+O")) {
                    try {
                        frames = OpenFrameSequence();
                        fps = frames.GetFramerate();
                    } catch (const std::invalid_argument &e) {
                    } catch (const std::runtime_error &e) {
                        currentError = std::string(e.what());
                        errorPopupOpen = true;
                    }
                }
                if (ImGui::MenuItem("Выйти", "Ctrl+Q")) {
                    done = true;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O,
                            ImGuiInputFlags_RouteGlobal)) {
            frames = OpenFrameSequence();
        }

        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Q,
                            ImGuiInputFlags_RouteGlobal)) {
            done = true;
        }

        if (ImGui::Begin("События")) {
            DisplayEvents(frames, currentIndex);
            ImGui::End();
        } else {
            ImGui::End();
        }

        if (ImGui::Begin("Таймлайн")) {
            DrawFrameSequenceTimeline("frames_timeline", frames, currentIndex,
                                      playing, fps, zoom);
            ImGui::End();
        } else {
            ImGui::End();
        }

        std::shared_ptr<CCTV::IGLTexture> texture;
        if (ImGui::Begin("Просмотр кадра", nullptr,
                         ImGuiWindowFlags_AlwaysAutoResize)) {
            if (frames.getLength() > 0) {
                CCTV::Frame frame = frames.get(currentIndex);
                texture = frame.GetTexture();

                ImGui::Text("Кадр: %d/ %d", currentIndex + 1,
                            frames.getLength());

                if (texture) {
                    ImGui::Image((ImTextureID)(intptr_t)texture->GetTexture(),
                                 ImVec2((float)frame.GetWidth(),
                                        (float)frame.GetHeight()));
                } else {
                    ImGui::TextUnformatted("Текстура не подгружена.");
                }
            } else {
                ImGui::TextUnformatted("Кадры не загружены.");
            }
            ImGui::End();
        } else {
            ImGui::End();
        }

        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
