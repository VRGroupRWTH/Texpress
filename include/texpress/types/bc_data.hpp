#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// Block compressed 2D data
struct bc2D
{
  std::vector<uint8_t>     data;
  glm::ivec2               size;
  glm::ivec2               blocks;
  std::size_t              img_len;
  std::size_t              data_len;
};

// Block compressed 3D data
struct bc3D
{
  std::vector<uint8_t>     data;
  glm::ivec3               size;
  glm::ivec3               blocks;
  std::size_t              img_len;
  std::size_t              data_len;
};
