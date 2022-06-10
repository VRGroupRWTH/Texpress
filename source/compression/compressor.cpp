#include <texpress/compression/compressor.hpp>
#include <spdlog/spdlog.h>

#include <BC.h>
#include <fp16.h>

namespace texpress {
  void Compressor::listener(const Event& e) {
    switch (e.mType) {
    case EventType::COMPRESS_BC6H:
      if (!e.mSendData) {
        spdlog::warn("No file has been uploaded!");
        return;
      }

      if (!e.mReceiveData) {
        spdlog::warn("No data destination provided!");
        return;
      }

      std::vector<uint8_t>* inData = static_cast<std::vector<uint8_t>*>(e.mSendData);
      std::vector<uint8_t>* outData = static_cast<std::vector<uint8_t>*>(e.mReceiveData);

      auto outData = convert_uint8_to_fp16(*inData);

      compress_bc6h(*inData, outData);
      return;
    }
  }

  bool Compressor::compress(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
    output.resize(input.size());
    std::copy(input.begin(), input.end(), output.data());
    spdlog::info("Compressed!");

    return true;
  }

  std::vector<uint16_t> Compressor::compress_bc6h(const image& input) {
    auto uint16 = convert_uint8_to_fp16(input.data);
    std::vector<uint8_t> output();
    bc6h_enc_settings settings{};

    GetProfile_bc6h_fast(&settings);

    ISPCTC_Surface_RGBA16F surface{};
    surface.Ptr    = (uint8_t*) uint16.data();
    surface.Width  = input.size.x;
    surface.Height = input.size.y;
    surface.Stride = 0;

    BC6HEncodeRGBA16F(&surface, )
  }

  std::vector<uint16_t> Compressor::convert_fp32_to_fp16(const std::vector<float>& input) {
    std::vector<uint16_t> fp16(input.size());
    for (uint64_t i = 0; i < input.size(); i++) {
      fp16[i] = fp16_ieee_from_fp32_value(input[i]);
    }

    return fp16;
  }

  std::vector<float> Compressor::convert_fp16_to_fp32(const std::vector<uint16_t>& input) {
    std::vector<float> fp32(input.size());
    for (uint64_t i = 0; i < input.size(); i++) {
      fp32[i] = fp16_ieee_to_fp32_value(input[i]);
    }

    return fp32;
  }

  std::vector<uint16_t> Compressor::convert_fp64_to_fp16(const std::vector<double>& input) {
    std::vector<uint16_t> fp16(input.size());
    for (uint64_t i = 0; i < input.size(); i++) {
      fp16[i] = fp16_ieee_from_fp32_value(static_cast<float>(input[i]));
    }

    return fp16;
  }

  std::vector<double> Compressor::convert_fp16_to_fp64(const std::vector<uint16_t>& input) {
    std::vector<double> fp64(input.size());
    for (uint64_t i = 0; i < input.size(); i++) {
      fp64[i] = static_cast<double>(fp16_ieee_to_fp32_value(input[i]));
    }

    return fp64;
  }

  std::vector<uint16_t> Compressor::convert_uint8_to_fp16(const std::vector<uint8_t>& input) {
    std::vector<uint16_t> fp16(input.size());
    for (uint64_t i = 0; i < input.size(); i++) {
      fp16[i] = fp16_ieee_from_fp32_value(static_cast<float>(input[i]) / 255.0f);
    }

    return fp16;
  }

  std::vector<uint8_t> Compressor::convert_fp16_to_uint8(const std::vector<uint16_t>& input) {
    std::vector<uint8_t> uint8(input.size());
    for (uint64_t i = 0; i < input.size(); i++) {
      uint8[i] = static_cast<uint8_t>(fp16_ieee_to_fp32_value(input[i]) * 255.0f);
    }

    return uint8;
  }
}