#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>

struct ldr_image
{
  std::vector<uint8_t> data;
  glm::ivec2           size;
  int                  channels;
};

struct hdr_image
{
  std::vector<float> data;
  glm::ivec2         size;
  int                channels;
};
