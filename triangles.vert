#version 460

layout (location = 0) in vec2 pos;

uniform float aspect;
uniform float scale;
uniform float rotation;
uniform vec2 translation;

void main() {
  mat2 rot = mat2(cos(-rotation), -sin(-rotation), sin(-rotation), cos(-rotation));
  gl_Position = vec4((rot * ((pos - translation) * scale)) * vec2(1., aspect), 0., 1.);
}
