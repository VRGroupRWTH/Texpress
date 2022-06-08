#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>

struct image
{
  std::vector<std::uint8_t> data;
  glm::ivec2                size;
};
