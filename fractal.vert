#version 460

in vec2 pos;

out vec2 v_pos;

uniform float aspect;

void main() {
  gl_Position = vec4(pos, 0., 1.);
  v_pos = pos / vec2(1., aspect);
}