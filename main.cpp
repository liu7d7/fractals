#include <iostream>
#include <glad.h>
#include <glfw3.h>
#include <vector>
#include "glm.hpp"
#include "glob.h"
#include "serpinski.h"
#include "koch.h"

using namespace glm;

int fractal = 0;

std::vector<std::string> fractals {
  "mandelbrot",
  "",
  ""
};

std::vector<void (*)(uint sh)> setters {
  [](uint sh) {},
};

std::vector<uint> shaders(fractals.size());
std::vector<triangle> tris;
std::vector<side> sides;

uint trivao, trivbo;

vec2 pos{0, 0};
float sc = 1.;
float lerped_sc = 1.;
float rotation = 0.;
float lerped_rotation = 0.;
int width = 1152, height = 720;

void reset() {
  tris.clear();
  sides.clear();
  constexpr float transy = -0.1f;
  tris.emplace_back(vec2(-0.5, -sqrt(3) / 6. + transy), vec2(0.5, -sqrt(3) / 6. + transy), vec2(0., sqrt(3) / 3. + transy));
  sides.emplace_back(tris[0].p3, tris[0].p1, 1);
  sides.emplace_back(tris[0].p1, tris[0].p2, 1);
  sides.emplace_back(tris[0].p2, tris[0].p3, 1);
  glNamedBufferData(trivbo, tris.size() * sizeof(triangle), tris.data(), GL_DYNAMIC_DRAW);
}

double last_divide = 0.;
uint divisions = 0;

int main() {
  if (!glfwInit()) {
    std::cout << "Failed to initialize GLFW" << std::endl;
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  glfwWindowHint(GLFW_SAMPLES, 4);
  GLFWwindow* win;
  if (!(win = glfwCreateWindow(width, height, "fractals", nullptr, nullptr))) {
    std::cout << "Failed to create window" << std::endl;
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(win);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    glfwTerminate();
    return -1;
  }

  glEnable(GL_MULTISAMPLE);

  for (int i = 0; i < 1; i++) {
    init_program(&shaders[i], "fractal.vert", fractals[i] + ".frag");
  }

  glfwSetKeyCallback(win, [](GLFWwindow* win, int keycode, int scancode, int action, int mods) {
    if ((keycode == GLFW_KEY_RIGHT || (keycode == GLFW_KEY_KP_6 && mods == GLFW_MOD_NUM_LOCK)) && action == GLFW_PRESS) {
      fractal = (fractal + 1) % fractals.size();
    }
    if ((keycode == GLFW_KEY_LEFT || (keycode == GLFW_KEY_KP_4 && mods == GLFW_MOD_NUM_LOCK)) && action == GLFW_PRESS) {
      fractal = fractal - 1 < 0 ? fractal - 1 + fractals.size() : fractal - 1;
    }
    if (((keycode == GLFW_KEY_LEFT || (keycode == GLFW_KEY_KP_4 && mods == GLFW_MOD_NUM_LOCK)) || (keycode == GLFW_KEY_RIGHT || (keycode == GLFW_KEY_KP_6 && mods == GLFW_MOD_NUM_LOCK)))) {
      last_divide = glfwGetTime();
      reset();
      divisions = 0;
      pos = {0, 0};
      sc = lerped_sc = 1.;
      lerped_rotation = rotation = 0.;
      if (fractal == 2) {
        pos = {0, -0.15};
        sc = lerped_sc = 1.25;
      }
    }
  });

  glfwSetWindowSizeCallback(win, [](GLFWwindow* win, int width, int height) {
    glViewport(0, 0, width, height);
    ::width = width;
    ::height = height;
  });

  glfwSetScrollCallback(win, [](GLFWwindow* win, double x, double y) {
    sc -= sign(y) * 0.05 * sc;
    sc = clamp(sc, 0.0001f, 15.f);
  });

  glViewport(0, 0, width, height);

  uint vao, vbo;
  uint trish;
  init_program(&trish, "triangles.vert", "triangles.frag");
  init_vao(&vao, &vbo, {2});
  init_vao(&trivao, &trivbo, {2});
  // 0 1 2 2 3 0
  glNamedBufferData(vbo, 6 * 2 * sizeof(float), (std::vector<float>{-1, -1, /**/ 1, -1, /**/ 1, 1, /**/ 1, 1, /**/ -1, 1, /**/ -1, -1}).data(), GL_DYNAMIC_DRAW);

  glClearColor(0.f, 0.f, 0.f, 1.f);
  glfwSwapInterval(1);
  float time = 0.;
  float last_time = 0.;
  while (!glfwWindowShouldClose(win)) {
    glClear(GL_COLOR_BUFFER_BIT);

    last_time = time;
    time = glfwGetTime();
    float delta = time - last_time;
    int front = glfwGetKey(win, GLFW_KEY_W) - glfwGetKey(win, GLFW_KEY_S),
        side  = glfwGetKey(win, GLFW_KEY_D) - glfwGetKey(win, GLFW_KEY_A);
    int left = glfwGetKey(win, GLFW_KEY_Q) - glfwGetKey(win, GLFW_KEY_E);
    mat2 rot = mat2(cos(rotation), -sin(rotation), sin(rotation), cos(rotation));
    pos += rot * vec2{side, front} * delta * sc;
    rotation -= left * delta;
    lerped_sc = mix(lerped_sc, sc, 0.2f);
    lerped_rotation = mix(lerped_rotation, rotation, 0.2f);

    if (fractal == 0) {
      uint p = shaders[fractal];
      glUseProgram(p);
      setters[fractal](p);
      glUniform1f(glGetUniformLocation(p, "time"), (float) glfwGetTime());
      glUniform1f(glGetUniformLocation(p, "aspect"), (float) width / (float) height);
      glUniform1f(glGetUniformLocation(p, "scale"), 1 / lerped_sc);
      glUniform2f(glGetUniformLocation(p, "translation"), pos.x, pos.y);
      glUniform1f(glGetUniformLocation(p, "rotation"), lerped_rotation);
      glBindVertexArray(vao);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    } else {
      if (glfwGetTime() - last_divide > 1.5 && fractal == 1) {
        if (divisions <= 12) {
          serpinski(tris);
          std::cout << "serp" << std::endl;
          glNamedBufferData(trivbo, tris.size() * sizeof(triangle), tris.data(), GL_DYNAMIC_DRAW);
        }
        last_divide = glfwGetTime();
        divisions++;
      }
      if (fractal == 2 && glfwGetTime() - last_divide > 1.5) {
       if (divisions <= 8) {
         koch(tris, sides);
         glNamedBufferData(trivbo, tris.size() * sizeof(triangle), tris.data(), GL_DYNAMIC_DRAW);
       }
        last_divide = glfwGetTime();
        divisions++;
      }
      glUseProgram(trish);
      glUniform1f(glGetUniformLocation(trish, "aspect"), (float) width / (float) height);
      glUniform1f(glGetUniformLocation(trish, "scale"), 1 / lerped_sc);
      glUniform2f(glGetUniformLocation(trish, "translation"), pos.x, pos.y);
      glUniform1f(glGetUniformLocation(trish, "rotation"), lerped_rotation);
      glBindVertexArray(trivao);
      glDrawArrays(GL_TRIANGLES, 0, tris.size() * 3);
    }

    glfwSwapBuffers(win);
    glfwPollEvents();
  }
  return 0;
}