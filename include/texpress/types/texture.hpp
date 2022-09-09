#pragma once

#include <cstdint>
#include <vector>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <glbinding/gl45core/enum.h>

namespace texpress
{
  struct Texture {
    std::vector<uint8_t> data;          // databuffer
    uint8_t channels = 0;
    glm::ivec4 dimensions = glm::ivec4(0);         // extents of each grid dimension
    gl::GLenum gl_type = gl::GLenum::GL_NONE;           // data type (unsigned int, float, ...) as glenum
    gl::GLenum gl_internal = gl::GLenum::GL_NONE; // internal format as glenum
    gl::GLenum gl_format = gl::GLenum::GL_NONE;
    glm::ivec3 enc_blocksize = glm::ivec3(0);     // extents of block dimensions (ignored for uncompressed)

    uint64_t bytes() const { 
      return data.size();
    }

    uint64_t bytes_type() const {
      switch (gl_type) {
      case gl::GLenum::GL_ZERO:
      case gl::GLenum::GL_BYTE:
      case gl::GLenum::GL_UNSIGNED_BYTE:
        return 1;
      case gl::GLenum::GL_SHORT:
      case gl::GLenum::GL_UNSIGNED_SHORT:
      case gl::GLenum::GL_HALF_FLOAT:
        return 2;
      case gl::GLenum::GL_INT:
      case gl::GLenum::GL_UNSIGNED_INT:
      case gl::GLenum::GL_FLOAT:
        return 4;
      case gl::GLenum::GL_DOUBLE:
        return 8;
      }

      // If not covered
      return 0;
    }

    bool compressed() const {
      int decimal = (int)gl_internal;
      // BC6H (and some other irrelevant compressions)
      if (decimal >= 36283 && decimal <= 36495) {
        return true;
      }

      // ASTC
      if (decimal >= 37808 && decimal <= 37853) {
        return true;
      }

      return false;
    }
  };

  inline gl::GLenum gl_format(int channels) {
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

  inline int gl_channels(gl::GLenum gl_format) {
    switch (gl_format) {
    case gl::GLenum::GL_R: case gl::GLenum::GL_RED: case gl::GLenum::GL_R8: case gl::GLenum::GL_R16: case gl::GLenum::GL_R16F: case gl::GLenum::GL_R32F:
      return 1;
    case gl::GLenum::GL_RG: case gl::GLenum::GL_RG8: case gl::GLenum::GL_RG16: case gl::GLenum::GL_RG16F: case gl::GLenum::GL_RG32F:
      return 2;
    case gl::GLenum::GL_RGB: case gl::GLenum::GL_RGB8: case gl::GLenum::GL_RGB16: case gl::GLenum::GL_RGB16F: case gl::GLenum::GL_RGB32F:
      return 3;
    case gl::GLenum::GL_RGBA: case gl::GLenum::GL_RGBA8: case gl::GLenum::GL_RGBA16: case gl::GLenum::GL_RGBA16F: case gl::GLenum::GL_RGBA32F:
      return 4;
    case gl::GLenum::GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
      return 3;
    case gl::GLenum::GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
      return 3;
    }

    return 0;
  }

  inline bool gl_compressed(gl::GLenum gl_internal) {
    int decimal = (int)gl_internal;
    // BC6H (and some other irrelevant compressions)
    if (decimal >= 36283 && decimal <= 36495) {
      return true;
    }

    // ASTC
    if (decimal >= 37808 && decimal <= 37853) {
      return true;
    }

    return false;
  }

  inline gl::GLenum gl_internal(int channels, int bits, bool hdr) {
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
  }

  inline gl::GLenum gl_type(gl::GLenum gl_internal) {
    switch (gl_internal) {
    case gl::GLenum::GL_R8:
    case gl::GLenum::GL_RG8:
    case gl::GLenum::GL_RGB8:
    case gl::GLenum::GL_RGBA8:
      return gl::GLenum::GL_UNSIGNED_BYTE;
    case gl::GLenum::GL_R16F:
    case gl::GLenum::GL_RG16F:
    case gl::GLenum::GL_RGB16F:
    case gl::GLenum::GL_RGBA16F:
      return gl::GLenum::GL_HALF_FLOAT;
    case gl::GLenum::GL_R32F:
    case gl::GLenum::GL_RG32F:
    case gl::GLenum::GL_RGB32F:
    case gl::GLenum::GL_RGBA32F:
      return gl::GLenum::GL_FLOAT;
    case gl::GLenum::GL_R32I:
    case gl::GLenum::GL_RG32I:
    case gl::GLenum::GL_RGB32I:
    case gl::GLenum::GL_RGBA32I:
      return gl::GLenum::GL_INT;
    }

    return gl::GLenum::GL_ZERO;
  }
}