#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "avs/audio.hpp"
#include "avs/effects.hpp"
#include "avs/engine.hpp"
#include "avs/fs.hpp"
#include "avs/preset.hpp"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "imgui_stdlib.h"

int main(int argc, char** argv) {
  std::filesystem::path presetDir = argc > 1 ? argv[1] : std::filesystem::current_path();

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    return 1;
  }
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_Window* window =
      SDL_CreateWindow("AVS Studio", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
                       SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  if (!window) {
    SDL_Quit();
    return 1;
  }
  SDL_GLContext ctx = SDL_GL_CreateContext(window);
  SDL_GL_SetSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForOpenGL(window, ctx);
  ImGui_ImplOpenGL3_Init("#version 330");

  avs::AudioInput audio;
  if (!audio.ok()) {
    return 1;
  }
  avs::Engine engine(640, 480);

  GLuint tex = 0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  int texW = 0, texH = 0;

  std::vector<std::filesystem::path> presets;
  for (auto& e : std::filesystem::directory_iterator(presetDir)) {
    if (e.is_regular_file() && e.path().extension() == ".avs") {
      presets.push_back(e.path());
    }
  }
  std::filesystem::path currentPreset;
  std::unique_ptr<avs::FileWatcher> watcher;
  std::vector<avs::Effect*> effects;
  avs::ScriptedEffect* scripted = nullptr;
  std::string frameSrc;
  std::string pixelSrc;

  auto loadPreset = [&](const std::filesystem::path& p) {
    auto parsed = avs::parsePreset(p);
    effects.clear();
    for (auto& e : parsed.chain) effects.push_back(e.get());
    engine.setChain(std::move(parsed.chain));
    currentPreset = p;
    watcher = std::make_unique<avs::FileWatcher>(p);
    scripted = nullptr;
    for (auto* eff : effects) {
      if (auto* se = dynamic_cast<avs::ScriptedEffect*>(eff)) {
        scripted = se;
        frameSrc.clear();
        pixelSrc.clear();
      }
    }
  };

  if (!presets.empty()) {
    loadPreset(presets[0]);
  }

  bool running = true;
  auto last = std::chrono::steady_clock::now();
  while (running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      ImGui_ImplSDL2_ProcessEvent(&e);
      if (e.type == SDL_QUIT) running = false;
    }

    if (watcher && watcher->poll()) {
      loadPreset(currentPreset);
    }

    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    last = now;

    auto audioState = audio.poll();
    engine.setAudio(audioState);

    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    engine.resize(w, h);
    engine.step(dt);

    const auto& fb = engine.frame();
    glBindTexture(GL_TEXTURE_2D, tex);
    if (fb.w != texW || fb.h != texH) {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fb.w, fb.h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                   fb.rgba.data());
      texW = fb.w;
      texH = fb.h;
    } else {
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fb.w, fb.h, GL_RGBA, GL_UNSIGNED_BYTE,
                      fb.rgba.data());
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport();

    ImGui::Begin("Presets");
    for (auto& p : presets) {
      bool sel = (p == currentPreset);
      if (ImGui::Selectable(p.filename().string().c_str(), sel)) {
        loadPreset(p);
      }
    }
    ImGui::End();

    ImGui::Begin("Properties");
    for (auto* eff : effects) {
      if (auto* blur = dynamic_cast<avs::BlurEffect*>(eff)) {
        int r = blur->radius();
        if (ImGui::SliderInt("Blur radius", &r, 1, 50)) {
          blur->setRadius(r);
        }
      } else if (auto* se = dynamic_cast<avs::ScriptedEffect*>(eff)) {
        scripted = se;
      }
    }
    if (scripted) {
      ImGui::InputTextMultiline("Frame", &frameSrc, ImVec2(-FLT_MIN, 100));
      ImGui::InputTextMultiline("Pixel", &pixelSrc, ImVec2(-FLT_MIN, 100));
      if (ImGui::Button("Compile")) {
        scripted->setScripts(frameSrc, pixelSrc);
      }
    }
    ImGui::End();

    ImGui::Begin("Scopes");
    if (!audioState.waveform.empty()) {
      ImGui::PlotLines("Waveform", audioState.waveform.data(),
                       static_cast<int>(audioState.waveform.size()), 0, nullptr, -1.0f, 1.0f,
                       ImVec2(0, 80));
    }
    if (!audioState.spectrum.empty()) {
      ImGui::PlotLines("Spectrum", audioState.spectrum.data(),
                       static_cast<int>(audioState.spectrum.size()), 0, nullptr, 0.0f, 1.0f,
                       ImVec2(0, 80));
    }
    ImGui::End();

    if (scripted && !scripted->lastError().empty()) {
      ImGui::Begin("Errors");
      ImGui::TextUnformatted(scripted->lastError().c_str());
      ImGui::End();
    }

    ImGui::Begin("Viewport");
    ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(tex)), ImVec2(fb.w, fb.h));
    ImGui::End();

    ImGui::Render();
    glViewport(0, 0, w, h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  glDeleteTextures(1, &tex);
  SDL_GL_DeleteContext(ctx);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
