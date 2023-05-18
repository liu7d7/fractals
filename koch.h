#pragma once

#include <vector>
#include "glob.h"
#include "geometric.hpp"

static void koch(std::vector<triangle>& tris, std::vector<side>& sides) {
  const float sqrt3 = sqrt(3);
  std::vector<side> new_sides;
  for (auto& old_side : sides) {
    float scale = pow(1.0 / 3.0, old_side.stage);
    vec2 perpendicular = perp(normalize(old_side.p1 - old_side.p2));
    vec2 a = old_side.p1 - (old_side.p1 - old_side.p2) * 1.0f / 3.0f;
    vec2 b = old_side.p1 - (old_side.p1 - old_side.p2) * 2.0f / 3.0f;
    vec2 c = old_side.p1 - (old_side.p1 - old_side.p2) * 0.5f + perpendicular * sqrt3 * 0.5f * scale;

    new_sides.emplace_back(a, c, old_side.stage + 1);
    new_sides.emplace_back(c, b, old_side.stage + 1);
    new_sides.emplace_back(old_side.p1, a, old_side.stage + 1);
    new_sides.emplace_back(b, old_side.p2, old_side.stage + 1);

    tris.emplace_back(a, b, c);
  }
  sides = new_sides;
}