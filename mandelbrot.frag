#version 460

in vec2 v_pos;

out vec4 f_color;

uniform float time;
uniform float rotation;
uniform vec2 translation;
uniform float scale;

vec3 cos_palette(float t, vec3 a, vec3 b, vec3 c, vec3 d) {
  return a + b*cos( 6.28318*(c*t+d) );
}

void main() {
  mat2 rot = mat2(cos(rotation), -sin(rotation), sin(rotation), cos(rotation));
  vec2 v0 = ((rot * v_pos) / scale * normalize(vec2(2.47, 2.24)) + translation);
  vec2 v = vec2(0.);
  int iter = 0;
  int max_iter = int(100 * pow(scale, 0.167));
  while (v.x * v.x + v.y * v.y <= 2 * 2 && iter++ < max_iter) {
    float xtemp = v.x * v.x - v.y * v.y + v0.x;
    v.y = 2 * v.x * v.y + v0.y;
    v.x = xtemp;
  }
  f_color = vec4(cos_palette(float(iter) / float(max_iter), vec3(0.8, 0.6, 1.), vec3(0.66, 0.82, 0.97), vec3(1., 1., 0.75), vec3(1., 0.69, 0.69)), 1.);
}