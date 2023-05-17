#version 460

in vec2 v_pos;

out vec4 f_color;

uniform float time;
uniform vec2 translation;
uniform float scale;
uniform float cx;
uniform float cy;
uniform float rotation;

const float R = 2;

vec3 cos_palette(float t, vec3 a, vec3 b, vec3 c, vec3 d) {
  return a + b*cos( 6.28318*(c*t+d) );
}

void main() {
  vec2 pos = v_pos * 2 - v_pos;
  vec2 z = pos * R;
  int iter = 0;
  int max_iter = 1000;

  while (z.x * z.x + z.y * z.y < R * R && iter < max_iter) {
    float xtemp = z.x * z.x - z.y * z.y;
    z.y = 2 * z.x * z.y + cy;
    z.x = xtemp + cx;
    iter++;
  }

  f_color = vec4(cos_palette(float(iter) / float(max_iter), vec3(0.8, 0.6, 1.), vec3(0.66, 0.82, 0.97), vec3(1., 1., 0.75), vec3(1., 0.69, 0.69)), 1.);
}