#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>

namespace texpress
{
  struct image {
    glm::ivec2 size;
    int        channels;

    virtual uint64_t bytes() = 0;
  };

  struct image_ldr : public image
  {
    std::vector<uint8_t> data;
    
    uint64_t bytes() { return data.size() * sizeof(uint8_t); }
  };

  struct image_hdr : public image
  {
    std::vector<float> data;

    uint64_t bytes() { return data.size() * sizeof(float); }
  };
}