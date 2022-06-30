#pragma once
#include <texpress/export.hpp>
#include <texpress/core/system.hpp>
#include <texpress/events/event.hpp>
#include <texpress/defines.hpp>
#include <texpress/types/image.hpp>
#include <texpress/types/bc_data.hpp>
#include <texpress/types/regular_grid.hpp>

#include <glbinding/gl45core/enum.h>
#include <texpress/compression/compressor_defines.hpp>
#include <glm/vec3.hpp>
#include <fp16.h>

namespace texpress
{
  struct TEXPRESS_EXPORT BlockCompressed {
    std::vector<uint8_t> data_ptr;    // databuffer
    uint64_t data_size;               // size of databuffer (redundant)
    glm::ivec4 grid_size;             // extents of each grid dimension
    gl::GLenum grid_glType;           // data type (unsigned int, float, ...) as glenum
    glm::ivec4 enc_blocks;            // number of blocks in each dimension
    glm::ivec3 enc_blocksize;         // extents of block dimensions
    gl::GLenum enc_glformat;          // compression format as glenum
  };

  struct TEXPRESS_EXPORT BC6H_options {
    float quality = 0.05;       // ignored for BC6H?
    uint8_t threads = 0;
    bool signed_data = true;
  };

  struct TEXPRESS_EXPORT BC7_options {
    float quality = 0.05;
    uint8_t threads = 0;
    bool restrictColor = false;
    bool restrictAlpha = false;
    uint8_t modeMask = 0xFF;
  };

  ldr_image TEXPRESS_EXPORT fit_blocksize(glm::ivec2 blocksize, const ldr_image& input);
  hdr_image TEXPRESS_EXPORT fit_blocksize(glm::ivec2 blocksize, const hdr_image& input);


  class TEXPRESS_EXPORT Encoder : public system
  {
  public:
    Encoder() = default;
    Encoder(const Encoder& that) = delete;
    Encoder(Encoder&& temp) = delete;
    ~Encoder() = default;
    Encoder& operator=(const Encoder& that) = delete;
    Encoder& operator=(Encoder&& temp) = delete;

  public:
    void listener(const Event& e);

  public:
    BlockCompressed compress_bc6h(const BC6H_options& options, const glm::ivec4& size, uint64_t bytes, uint64_t offset, const uint8_t* input);
    BlockCompressed compress_bc6h(const hdr_image& input, const BC6H_options& options = {});
    BlockCompressed compress_bc6h(const ldr_image& input, const BC6H_options& options);
    BlockCompressed compress_bc6h(const grid2& input, const BC6H_options& options);
    BlockCompressed compress_bc6h(const grid3& input, const BC6H_options& options);
    BlockCompressed compress_bc6h(const grid4& input, const BC6H_options& options);

    BlockCompressed compress_bc7(const BC7_options& options, const glm::ivec4& size, uint64_t bytes, uint64_t offset, const uint8_t* input);
    BlockCompressed compress_bc7(const hdr_image& input, const BC7_options& options);
    BlockCompressed compress_bc7(const ldr_image& input, const BC7_options& options);
    BlockCompressed compress_bc7(const grid2& input, const BC7_options& options);
    BlockCompressed compress_bc7(const grid3& input, const BC7_options& options);
    BlockCompressed compress_bc7(const grid4& input, const BC7_options& options);

    BlockCompressed compress_bc6h_legacy(const hdr_image& input);
    BlockCompressed compress_bc7_legacy(const ldr_image& input);
  private:
    void compress_bc7_blocks(const glm::ivec4 size, const uint8_t* input_ptr, uint8_t* output_ptr);

  public:

    template <typename T>
    std::vector<uint16_t> convert_to_f16(uint64_t offset, const std::vector<T>& input) {
      /*
      std::vector<uint16_t> f16(input.size() - offset);
      for (uint64_t i = offset; i < input.size(); i++) {
        f16[i] = fp16_ieee_from_fp32_value(static_cast<float>(input[i]));
      }

      return f16;
      */

      return convert_to_f16<T>(input.size() * sizeof(T), offset * sizeof(T), input.data());

    }

    template <typename T>
    void convert_to_f16(uint64_t size_bytes, uint64_t offset_bytes, const T* input, uint16_t* output) {
      uint64_t elements = size_bytes / sizeof(T);
      uint64_t skip_elements = offset_bytes / sizeof(T);
      uint64_t proc_elements = elements - skip_elements;

      // Delete any allocated data and reallocate
      delete[] output;
      output = new uint16_t[proc_elements];
      for (uint64_t i = 0; i < proc_elements; i++) {
        output[i] = fp16_ieee_from_fp32_value(static_cast<float>(*(input + skip_elements + i)));
      }
    }

    template <typename T>
    std::vector<T> convert_from_f16(uint64_t offset, const std::vector<uint16_t>& input) {
      /*
      std::vector<uint16_t> values(input.size() - offset);
      for (uint64_t i = offset; i < input.size(); i++) {
        values[i] = fp16_ieee_to_fp32_value(input[i]);
      }

      return values;
      */

      return convert_from_f16<T>(input.size() * sizeof(uint16_t), offset * sizeof(uint16_t), input.data());
    }

    template <typename T>
    void convert_from_f16(uint64_t size_bytes, uint64_t offset_bytes, const uint16_t* input, T* output) {
      uint64_t elements = size_bytes / sizeof(uint16_t);
      uint64_t skip_elements = offset_bytes / sizeof(uint16_t);
      uint64_t proc_elements = elements - skip_elements;

      // Delete any allocated data and reallocate
      delete[] output;
      output = new T[proc_elements];
      for (uint64_t i = 0; i < proc_elements; i++) {
        output[i] = fp16_ieee_to_fp32_value(*(input + skip_elements + i));
      }

      return values;
    }
  };
}