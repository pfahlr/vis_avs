#include "avs/window.hpp"

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <unordered_set>

namespace avs {

namespace {

GLuint Compile(GLenum type, const char* src) {
  GLuint sh = glCreateShader(type);
  glShaderSource(sh, 1, &src, nullptr);
  glCompileShader(sh);
  GLint ok = 0;
  glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
    glDeleteShader(sh);
    throw std::runtime_error(log);
  }
  return sh;
}

GLuint Link(GLuint vs, GLuint fs) {
  GLuint prog = glCreateProgram();
  glAttachShader(prog, vs);
  glAttachShader(prog, fs);
  glLinkProgram(prog);
  GLint ok = 0;
  glGetProgramiv(prog, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
    glDeleteProgram(prog);
    throw std::runtime_error(log);
  }
  glDetachShader(prog, vs);
  glDetachShader(prog, fs);
  glDeleteShader(vs);
  glDeleteShader(fs);
  return prog;
}

}  // namespace

struct Window::Impl {
  SDL_Window* win = nullptr;
  SDL_GLContext ctx = nullptr;
  GLuint tex = 0;
  GLuint prog = 0;
  GLuint vao = 0;
  GLuint vbo = 0;
  int w = 0;
  int h = 0;
  int tex_w = 0;
  int tex_h = 0;
  std::unordered_set<int> keys;
};

Window::Window(int w, int h, const char* title) : impl_(std::make_unique<Impl>()) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    throw std::runtime_error(SDL_GetError());
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  impl_->win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  if (!impl_->win) {
    throw std::runtime_error(SDL_GetError());
  }
  impl_->ctx = SDL_GL_CreateContext(impl_->win);
  if (!impl_->ctx) {
    throw std::runtime_error(SDL_GetError());
  }
  SDL_GL_SetSwapInterval(1);

  impl_->w = w;
  impl_->h = h;
  glViewport(0, 0, w, h);

  const char* vs_src = R"(#version 330 core
layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_uv;
out vec2 uv;
void main() {
  uv = in_uv;
  gl_Position = vec4(in_pos, 0.0, 1.0);
})";
  const char* fs_src = R"(#version 330 core
in vec2 uv;
out vec4 color;
uniform sampler2D u_tex;
void main() {
  color = texture(u_tex, uv);
})";
  GLuint vs = Compile(GL_VERTEX_SHADER, vs_src);
  GLuint fs = Compile(GL_FRAGMENT_SHADER, fs_src);
  impl_->prog = Link(vs, fs);
  glUseProgram(impl_->prog);
  glUniform1i(glGetUniformLocation(impl_->prog, "u_tex"), 0);

  glGenVertexArrays(1, &impl_->vao);
  glBindVertexArray(impl_->vao);
  glGenBuffers(1, &impl_->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, impl_->vbo);
  float verts[] = {
      -1.f, -1.f, 0.f, 0.f, 1.f, -1.f, 1.f, 0.f, -1.f, 1.f, 0.f, 1.f, 1.f, 1.f, 1.f, 1.f,
  };
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        reinterpret_cast<const void*>(static_cast<std::uintptr_t>(2 * sizeof(float))));

  glGenTextures(1, &impl_->tex);
  glBindTexture(GL_TEXTURE_2D, impl_->tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

Window::~Window() {
  if (!impl_) return;
  glDeleteTextures(1, &impl_->tex);
  glDeleteBuffers(1, &impl_->vbo);
  glDeleteVertexArrays(1, &impl_->vao);
  glDeleteProgram(impl_->prog);
  SDL_GL_DeleteContext(impl_->ctx);
  SDL_DestroyWindow(impl_->win);
  SDL_Quit();
}

bool Window::poll() {
  impl_->keys.clear();
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) return false;
    if (e.type == SDL_WINDOWEVENT && (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                                      e.window.event == SDL_WINDOWEVENT_RESIZED)) {
      impl_->w = e.window.data1;
      impl_->h = e.window.data2;
      glViewport(0, 0, impl_->w, impl_->h);
    }
    if (e.type == SDL_KEYDOWN) {
      impl_->keys.insert(e.key.keysym.sym);
    }
  }
  return true;
}

std::pair<int, int> Window::size() const { return {impl_->w, impl_->h}; }

void Window::blit(const std::uint8_t* rgba, int width, int height) {
  glBindTexture(GL_TEXTURE_2D, impl_->tex);
  if (width != impl_->tex_w || height != impl_->tex_h) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    impl_->tex_w = width;
    impl_->tex_h = height;
  }
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(impl_->prog);
  glBindVertexArray(impl_->vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  SDL_GL_SwapWindow(impl_->win);
}

bool Window::keyPressed(int key) {
  auto it = impl_->keys.find(key);
  if (it != impl_->keys.end()) {
    impl_->keys.erase(it);
    return true;
  }
  return false;
}

}  // namespace avs
