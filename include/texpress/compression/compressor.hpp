#pragma once
#include <texpress/export.hpp>
#include <texpress/core/system.hpp>
#include <texpress/events/event.hpp>
#include <texpress/defines.hpp>
#include <texpress/types/image.hpp>
#include <texpress/types/bc_data.hpp>
#include <texpress/types/regular_grid.hpp>

#include <glm/vec3.hpp>
#include <fp16.h>

namespace texpress
{
  enum CompressionFormat {
    BC6H,
    BC7,
    ASTC
  };

  enum DataType {
    UINT8,
    INT8,
    UFLOAT32,
    FLOAT32
  };

  struct BlockCompressed {
    std::vector<uint8_t> data_ptr;
    uint64_t data_size;
    glm::ivec4 grid_size;
    DataType grid_type;
    glm::ivec4 enc_blocks;
    glm::ivec3 enc_blocksize;
    CompressionFormat compression_format;
  };

  class TEXPRESS_EXPORT Encoder : public system
  {
  public:
    Encoder();
    Encoder(const Encoder& that) = delete;
    Encoder(Encoder&& temp) = delete;
    ~Encoder() = default;
    Encoder& operator=(const Encoder& that) = delete;
    Encoder& operator=(Encoder&& temp) = delete;

  public:
    void listener(const Event& e);

  private:
    static bool initialized;

  public:
    BlockCompressed compress_bc6h(const glm::ivec3& size, uint64_t bytes, uint64_t offset, const uint16_t* input);
    BlockCompressed compress_bc6h(const hdr_image& input);
    BlockCompressed compress_bc6h(const grid2& input);
    BlockCompressed compress_bc6h(const grid3& input);
    BlockCompressed compress_bc6h(const grid4& input);

    /*
    std::vector<uint8_t> compress_astc(const glm::ivec3& size, uint64_t bytes, uint64_t offset, const uint16_t* input);
    std::vector<uint8_t> compress_astc(const hdr_image& input);
    std::vector<uint8_t> compress_astc(const grid2& input);
    std::vector<uint8_t> compress_astc(const grid3& input);
    std::vector<uint8_t> compress_astc(const grid4& input);
    */

    /*
    std::vector<uint8_t> compress_bc7(const glm::ivec3& size, uint64_t bytes, uint64_t offset, const uint8_t* input);
    std::vector<uint8_t> compress_bc7(const ldr_image& input);
    std::vector<uint8_t> compress_bc7(const grid2& input);
    std::vector<uint8_t> compress_bc7(const grid3& input);
    std::vector<uint8_t> compress_bc7(const grid4& input);
    */

    hdr_image canonical_shape(CompressionFormat format, const hdr_image& input);

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