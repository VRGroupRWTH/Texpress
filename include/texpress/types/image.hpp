#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>

namespace texpress
{
    struct image {
        std::vector<uint8_t> data;
        glm::ivec2 size;
        int        channels;
        bool       hdr;

        uint64_t bytes() const { return data.size(); }
        bool is_hdr() const { return hdr; }
    };
}