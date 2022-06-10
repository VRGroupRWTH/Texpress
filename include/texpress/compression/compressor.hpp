#pragma once
#include <texpress/export.hpp>
#include <texpress/core/system.hpp>
#include <texpress/events/event.hpp>
#include <texpress/defines.hpp>
#include <texpress/types/image.hpp>
#include <texpress/types/bc_data.hpp>
#include <texpress/types/regular_grid.hpp>

namespace texpress
{
  class TEXPRESS_EXPORT Compressor : public system
  {
  public:
    Compressor() = default;
    Compressor(const Compressor& that) = delete;
    Compressor(Compressor&& temp) = delete;
    ~Compressor() = default;
    Compressor& operator=(const Compressor& that) = delete;
    Compressor& operator=(Compressor&& temp) = delete;

  public:
    void listener(const Event& e);

  public:
    std::vector<uint8_t> compress_bc6h(const image& input);
    std::vector<uint8_t> compress_bc6h();

    std::vector<uint8_t> compress_bc7(const image& input);
    std::vector<uint8_t> compress_bc7();

  private:
    std::vector<uint16_t> convert_fp64_to_fp16(const std::vector<double>& input);
    std::vector<uint16_t> convert_fp32_to_fp16(const std::vector<float>& input);
    std::vector<double> convert_fp16_to_fp64(const std::vector<uint16_t>& input);
    std::vector<float> convert_fp16_to_fp32(const std::vector<uint16_t>& input);

    std::vector<uint16_t> convert_uint8_to_fp16(const std::vector<uint8_t>& input);
    std::vector<uint8_t> convert_fp16_to_uint8(const std::vector<uint16_t>& input);
  };
}