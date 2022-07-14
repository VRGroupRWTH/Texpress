#pragma once

#include <cstdint>
#include <vector>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <glbinding/gl45core/enum.h>

namespace texpress
{
  template <typename T>
  struct Texture {
    std::vector<T> data;              // databuffer
    uint64_t data_size;               // size of databuffer (redundant)
    uint8_t data_channels;
    glm::ivec4 grid_size;             // extents of each grid dimension
    gl::GLenum grid_glType;           // data type (unsigned int, float, ...) as glenum
    gl::GLenum enc_glformat;          // compression format as glenum (ignored for uncompressed)
    glm::ivec3 enc_blocksize;         // extents of block dimensions (ignored for uncompressed)
  };
}