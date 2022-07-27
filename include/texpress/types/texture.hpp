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
    std::vector<T> data;          // databuffer
    uint64_t data_size;           // size of databuffer (redundant)
    uint8_t data_channels;
    glm::ivec4 grid_size;         // extents of each grid dimension
    gl::GLenum gl_type;           // data type (unsigned int, float, ...) as glenum
    gl::GLenum gl_internalFormat; // internal format as glenum
    gl::GLenum gl_pixelFormat;
    glm::ivec3 enc_blocksize;     // extents of block dimensions (ignored for uncompressed)

    uint64_t bytes() { return data.size() * sizeof(T); }
  };

  inline gl::GLenum gl_pixel(int channels) {
    switch (channels) {
    case 1:
      return gl::GLenum::GL_R;
    case 2:
      return gl::GLenum::GL_RG;
    case 3:
      return gl::GLenum::GL_RGB;
    case 4:
      return gl::GLenum::GL_RGBA;
    }
  };

  inline int gl_colorchannels(gl::GLenum gl_format) {
    switch (gl_format) {
    case gl::GLenum::GL_R:
      return 1;
    case gl::GLenum::GL_RG:
      return 2;
    case gl::GLenum::GL_RGB:
      return 3;
    case gl::GLenum::GL_RGBA:
      return 4;
    case gl::GLenum::GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
      return 4;
    case gl::GLenum::GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
      return 4;
    }

    return 4;
  }

  inline gl::GLenum gl_internal_uncomnpressed(int channels, int bits, bool hdr) {
    if (bits == 8) {
      if (!hdr) {
        switch (channels) {
        case 1:
          return gl::GLenum::GL_R8;
        case 2:
          return gl::GLenum::GL_RG8;
        case 3:
          return gl::GLenum::GL_RGB8;
        case 4:
          return gl::GLenum::GL_RGBA8;
        }
      }
      else {
        // hdr needs 16 bits minimum
        return gl::GLenum::GL_NONE;
      }
    }

    if (bits == 16) {
      if (!hdr) {
        switch (channels) {
        case 1:
          return gl::GLenum::GL_R16;
        case 2:
          return gl::GLenum::GL_RG16;
        case 3:
          return gl::GLenum::GL_RGB16;
        case 4:
          return gl::GLenum::GL_RGBA16;
        }
      }
      else {
        switch (channels) {
        case 1:
          return gl::GLenum::GL_R16F;
        case 2:
          return gl::GLenum::GL_RG16F;
        case 3:
          return gl::GLenum::GL_RGB16F;
        case 4:
          return gl::GLenum::GL_RGBA16F;
        }
      }
    }

    if (bits == 32) {
      if (!hdr) {
        switch (channels) {
        case 1:
          return gl::GLenum::GL_R32I;
        case 2:
          return gl::GLenum::GL_RG32I;
        case 3:
          return gl::GLenum::GL_RGB32I;
        case 4:
          return gl::GLenum::GL_RGBA32I;
        }
      }
      else {
        switch (channels) {
        case 1:
          return gl::GLenum::GL_R32F;
        case 2:
          return gl::GLenum::GL_RG32F;
        case 3:
          return gl::GLenum::GL_RGB32F;
        case 4:
          return gl::GLenum::GL_RGBA32F;
        }
      }
    }
  };
}