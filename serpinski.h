#pragma once

#include <vector>
#include "glob.h"

static void serpinski(std::vector<triangle>& tris) {
  std::vector<triangle> newTris(tris.size() * 3);

  for (int i = 0; i < tris.size(); i++) {
    vec2 a = tris[i].p1, b = tris[i].p2, c = tris[i].p3;

    newTris[i * 3 + 0] = triangle(a, a - (a - b) * 0.5f, a - (a - c) * 0.5f);
    newTris[i * 3 + 1] = triangle(b, b - (b - a) * 0.5f, b - (b - c) * 0.5f);
    newTris[i * 3 + 2] = triangle(c, c - (c - a) * 0.5f, c - (c - b) * 0.5f);
  }

  tris = newTris;
} 