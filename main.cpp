#include <iostream>
#include <glad.h>
#include <glfw3.h>
#include <vector>
#include <numeric>
#include <fstream>
#include <complex>
#include "glm.hpp"

using namespace glm;

int fractal = 0;
struct triangle {
  vec2 p1, p2, p3;
  triangle() : p1(0.), p2(0.), p3(0.) {}
  triangle(vec2 p1, vec2 p2, vec2 p3) : p1(p1), p2(p2), p3(p3) {}
};
struct side {
  vec2 p1, p2;
  int stage;
  side() : p1(0.), p2(0.), stage(0) {}
  side(vec2 p1, vec2 p2, int stage) : p1(p1), p2(p2), stage(stage) {}
};
std::vector<std::string> fractals {
  "mandelbrot",
  "julia",
  "",
  ""
};
std::vector<void (*)(uint sh)> setters {
  [](uint sh) {},
  [](uint sh) {
    std::complex<double> a(0.7885);
    a *= pow(std::complex<double>(2.71828'18284'59045'23536), std::complex<double>(0., fmod(glfwGetTime() * 0.33, M_PI * 2.)));
    glUniform1f(glGetUniformLocation(sh, "cx"), (float)a.real());
    glUniform1f(glGetUniformLocation(sh, "cy"), (float)a.imag());
  },
};
std::vector<uint> shaders(fractals.size());
std::vector<triangle> tris;
vec2 pos{0, 0};
float sc = 1.;
float realsc = 1.;
int width = 1152, height = 720;
float rotation = 0.;
float realrotation = 0.;
uint trivao, trivbo;
uint stage;
std::vector<side> sides;

void reset() {
  tris.clear();
  sides.clear();
  constexpr float transy = -0.1f;
  tris.emplace_back(vec2(-0.5, -0.29 + transy), vec2(0.5, -0.29 + transy), vec2(0., 0.58 + transy));
  sides.emplace_back(tris[0].p3, tris[0].p1, 2);
  sides.emplace_back(tris[0].p1, tris[0].p2, 2);
  sides.emplace_back(tris[0].p2, tris[0].p3, 2);
}

void subdivide() {
  std::vector<triangle> newTris(tris.size() * 3);
  for (int i = 0; i < tris.size(); i++) {
    vec2 a = tris[i].p1, b = tris[i].p2, c = tris[i].p3;
    newTris[i * 3 + 0] = triangle(a, a - (a - b) * 0.5f, a - (a - c) * 0.5f);
    newTris[i * 3 + 1] = triangle(b, b - (b - a) * 0.5f, b - (b - c) * 0.5f);
    newTris[i * 3 + 2] = triangle(c, c - (c - a) * 0.5f, c - (c - b) * 0.5f);
  }
  tris = newTris;
}

vec2 perp(vec2 in) {
  return {-in.y, in.x};
}

void koch() {
  std::vector<side> newSides;
  for (auto& it : sides) {
    vec2 a = it.p1 - (it.p1 - it.p2) * 0.33f;
    vec2 b = it.p1 - (it.p1 - it.p2) * 0.66f;
    vec2 perpendicular = perp(normalize(it.p1 - it.p2));
    vec2 c = it.p1 - (it.p1 - it.p2) * 0.5f + perpendicular * (float)sqrt(3) * 0.5f * (float)pow(0.3333333333333333333333, it.stage - 1);
    tris.emplace_back(a, b, c);
    newSides.emplace_back(a, c, it.stage + 1);
    newSides.emplace_back(c, b, it.stage + 1);
    newSides.emplace_back(it.p1, a, it.stage + 1);
    newSides.emplace_back(b, it.p2, it.stage + 1);
  }
  stage++;
  sides = newSides;
}

static void g_initvao(uint* vao, uint* vbo, const std::vector<int>& attribs) {
  glCreateVertexArrays(1, vao);
  glCreateBuffers(1, vbo);

  int stride = std::accumulate(attribs.begin(), attribs.end(), 0);
  glVertexArrayVertexBuffer(*vao, 0, *vbo, 0, stride * sizeof(float));

  int offset = 0;
  for (int i = 0; i < attribs.size(); i++) {
    glEnableVertexArrayAttrib(*vao, i);
    glVertexArrayAttribFormat(*vao, i, attribs[i], GL_FLOAT, GL_FALSE, offset * sizeof(float));
    glVertexArrayAttribBinding(*vao, i, 0);
    offset += attribs[i];
  }
}

static void g_initprogram(uint* prog, const std::string& vs, const std::string& fs, const std::string& gs = "") {
  std::string vss, fss, gss;
  std::ifstream vs_file(vs);
  std::ifstream fs_file(fs);
  if (!vs_file.is_open() || !fs_file.is_open()) throw std::runtime_error("failed to open shader files");
  vss = std::string(std::istreambuf_iterator<char>(vs_file), std::istreambuf_iterator<char>());
  fss = std::string(std::istreambuf_iterator<char>(fs_file), std::istreambuf_iterator<char>());
  vs_file.close();
  fs_file.close();

  if (!gs.empty()) {
    std::ifstream gs_file(gs);
    if (!gs_file.is_open()) throw std::runtime_error("failed to open shader files");
    gss = std::string(std::istreambuf_iterator<char>(gs_file), std::istreambuf_iterator<char>());
    gs_file.close();
  }

  const char* vs_src = vss.c_str();
  const char* fs_src = fss.c_str();
  const char* gs_src = gss.c_str();

  uint vsh = glCreateShader(GL_VERTEX_SHADER), fsh = glCreateShader(GL_FRAGMENT_SHADER);
  uint gsh = 0;
  if (!gs.empty()) gsh = glCreateShader(GL_GEOMETRY_SHADER);

  glShaderSource(vsh, 1, &vs_src, nullptr);
  glShaderSource(fsh, 1, &fs_src, nullptr);
  if (!gs.empty()) glShaderSource(gsh, 1, &gs_src, nullptr);

  int status;

  glCompileShader(vsh);
  glGetShaderiv(vsh, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    char log[512];
    glGetShaderInfoLog(vsh, 512, nullptr, log);
    throw std::runtime_error(std::string("failed to compile vertex shader: ") + log);
  }
  glCompileShader(fsh);
  glGetShaderiv(fsh, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    char log[512];
    glGetShaderInfoLog(fsh, 512, nullptr, log);
    throw std::runtime_error(std::string("failed to compile fragment shader: ") + log);
  }
  if (!gs.empty()) {
    glCompileShader(gsh);
    glGetShaderiv(gsh, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
      char log[512];
      glGetShaderInfoLog(gsh, 512, nullptr, log);
      throw std::runtime_error(std::string("failed to compile geometry shader: ") + log);
    }
  }

  *prog = glCreateProgram();

  glAttachShader(*prog, vsh);
  glAttachShader(*prog, fsh);
  if (!gs.empty()) glAttachShader(*prog, gsh);

  glLinkProgram(*prog);
  glGetProgramiv(*prog, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    char log[512];
    glGetProgramInfoLog(*prog, 512, nullptr, log);
    throw std::runtime_error(std::string("failed to link program: ") + log);
  }

  glDeleteShader(vsh);
  glDeleteShader(fsh);
  if (!gs.empty()) glDeleteShader(gsh);
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

  for (int i = 0; i < 2; i++) {
    g_initprogram(&shaders[i], "fractal.vert", fractals[i] + ".frag");
  }

  glfwSetKeyCallback(win, [](GLFWwindow* win, int keycode, int scancode, int action, int mods) {
    if ((keycode == GLFW_KEY_RIGHT || (keycode == GLFW_KEY_KP_6 && mods == GLFW_MOD_NUM_LOCK)) && action == GLFW_PRESS) {
      fractal = (fractal + 1) % fractals.size();
      last_divide = glfwGetTime();
      reset();
      divisions = 0;
      pos = {0, 0};
      sc = realsc = 1.;
      realrotation = rotation = 0.;
    }
    if ((keycode == GLFW_KEY_LEFT || (keycode == GLFW_KEY_KP_4 && mods == GLFW_MOD_NUM_LOCK)) && action == GLFW_PRESS) {
      fractal = fractal - 1 < 0 ? fractal - 1 + fractals.size() : fractal - 1;
      last_divide = glfwGetTime();
      reset();
      divisions = 0;
      pos = {0, 0};
      sc = realsc = 1.;
      realrotation = rotation = 0.;
    }
    if (fractal == 3 && ((keycode == GLFW_KEY_LEFT || (keycode == GLFW_KEY_KP_4 && mods == GLFW_MOD_NUM_LOCK)) || (keycode == GLFW_KEY_RIGHT || (keycode == GLFW_KEY_KP_6 && mods == GLFW_MOD_NUM_LOCK)))) {
      pos = {0, -0.15};
      sc = realsc = 1.25;
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
  g_initprogram(&trish, "triangles.vert", "triangles.frag");
  g_initvao(&vao, &vbo, { 2 });
  g_initvao(&trivao, &trivbo, { 2 });
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
    realsc = mix(realsc, sc, 0.2f);
    realrotation = mix(realrotation, rotation, 0.2f);

    if (fractal <= 1) {
      uint p = shaders[fractal];
      glUseProgram(p);
      setters[fractal](p);
      glUniform1f(glGetUniformLocation(p, "time"), (float) glfwGetTime());
      glUniform1f(glGetUniformLocation(p, "aspect"), (float) width / (float) height);
      glUniform1f(glGetUniformLocation(p, "scale"), 1 / realsc);
      glUniform2f(glGetUniformLocation(p, "translation"), pos.x, pos.y);
      glUniform1f(glGetUniformLocation(p, "rotation"), realrotation);
      glBindVertexArray(vao);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    } else {
      if (glfwGetTime() - last_divide > 0.5 && fractal == 2) {
        if (divisions <= 12)
          subdivide();
        last_divide = glfwGetTime();
        std::cout << "sub" << std::endl;
        divisions++;
      }
      if (fractal == 3 && glfwGetTime() - last_divide > 2) {
       if (divisions <= 8)
          koch();
        last_divide = glfwGetTime();
        std::cout << "koch" << std::endl;
        divisions++;
      }
      glNamedBufferData(trivbo, tris.size() * sizeof(triangle), tris.data(), GL_DYNAMIC_DRAW);
      glUseProgram(trish);
      glUniform1f(glGetUniformLocation(trish, "aspect"), (float) width / (float) height);
      glUniform1f(glGetUniformLocation(trish, "scale"), 1 / realsc);
      glUniform2f(glGetUniformLocation(trish, "translation"), pos.x, pos.y);
      glUniform1f(glGetUniformLocation(trish, "rotation"), realrotation);
      glBindVertexArray(trivao);
      glDrawArrays(GL_TRIANGLES, 0, tris.size() * 3);
    }

    glfwSwapBuffers(win);
    glfwPollEvents();
  }
  return 0;
}