#include <texpress/compression/compressor.hpp>
#include <spdlog/spdlog.h>

#include <compressonator.h>

namespace texpress {
  bool Encoder::initialized = false;

  Encoder::Encoder() {
    //--------------------------
    // Init frameworks
    // plugin and IO interfaces
    //--------------------------
    if (!initialized)
      //CMP_InitFramework();

    initialized = true;
  }

  void Encoder::listener(const Event& e) {
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
      return;
    }
  }

  BlockCompressed Encoder::compress_bc6h(const hdr_image& input) {
    uint64_t pixels = input.size.x * input.size.y;
    auto canonical = canonical_shape(CompressionFormat::BC6H, input);
    CMP_Texture srcTexture;
    srcTexture.dwSize = sizeof(srcTexture);
    srcTexture.dwWidth = srcTexture.dwWidth;
    srcTexture.dwHeight = srcTexture.dwHeight;
    srcTexture.dwPitch = 0;
    srcTexture.format = CMP_FORMAT_RGBA_16F;
    srcTexture.dwDataSize = CMP_CalculateBufferSize(&srcTexture);
    srcTexture.pData = (CMP_BYTE*)malloc(srcTexture.dwDataSize);

    CMP_Texture dstTexture;
    dstTexture.dwSize = sizeof(dstTexture);
    dstTexture.dwWidth = dstTexture.dwWidth;
    dstTexture.dwHeight = dstTexture.dwHeight;
    dstTexture.dwPitch = 0;
    dstTexture.format = CMP_FORMAT_BC6H_SF;
    dstTexture.dwDataSize = CMP_CalculateBufferSize(&dstTexture);
    dstTexture.pData = (CMP_BYTE*)malloc(dstTexture.dwDataSize);

    BlockCompressed encoded{};
    encoded.data.resize(pixels); // 8 bits per texel
    encoded.data_size = { canonical.size, 1, 1 }; // images are assumed 2D
    encoded.data_type = DataType::FLOAT32; // HDR
    encoded.compression_format = CompressionFormat::BC6H;
    encoded.block_size = { 4, 4, 1 }; // fixed blocksize for BC6H
    encoded.blocks = encoded.data_size / glm::ivec4(encoded.block_size, 1);

    if (input.channels != 3) {
      auto f16 = std::vector<uint16_t>(canonical.data.size());

      for (uint64_t pixel = 0; pixel < pixels; pixel++) {
        f16[pixel * 4 + 0] = fp16_ieee_from_fp32_value((canonical.channels >= 1) ? input.data[pixel * input.channels + 0] : 0.f);
        f16[pixel * 4 + 1] = fp16_ieee_from_fp32_value((canonical.channels >= 2) ? input.data[pixel * input.channels + 1] : 0.f);
        f16[pixel * 4 + 2] = fp16_ieee_from_fp32_value((canonical.channels >= 3) ? input.data[pixel * input.channels + 2] : 0.f);
      }

      //BC6HEncodeRGBA16F(&surface, encoded.data.data(), &settings);
    }
    else {
      auto f16 = convert_to_f16(0, input.data);

      //BC6HEncodeRGBA16F(&surface, encoded.data.data(), &settings);
    }

    return encoded;
  }

hdr_image Encoder::canonical_shape(CompressionFormat format, const hdr_image& input) {
  if (format == CompressionFormat::BC6H) {
    glm::ivec2 target_blocksize(4, 4);
    // If image size is not a multiple of blocksize
    // this will yield the gap of missing pixels to fill
    glm::ivec2 block_remainder = target_blocksize - (input.size % target_blocksize);
    block_remainder.x = (block_remainder.x == target_blocksize.x) ? 0 : block_remainder.x;
    block_remainder.y = (block_remainder.y == target_blocksize.y) ? 0 : block_remainder.y;

    if ((block_remainder.x != 0) || (block_remainder.y != 0)) {
      hdr_image out{ {}, input.size + block_remainder, input.channels };
      uint64_t out_pixels = out.size.x * out.size.y;
      out.data.resize(out_pixels * out.channels);

      uint64_t in_pos = 0;
      uint64_t out_pos = 0;
      for (uint64_t y = 0; y < input.size.y; y++) {
        for (uint64_t x = 0; x < input.size.x; x++) {
          for (uint64_t c = 0; c < input.channels; c++) {
            // Copy center left
            out.data[out_pos] = input.data[in_pos];
            out_pos++;
            in_pos++;
          }
        }

        for (uint64_t x = 0; x < block_remainder.x; x++) {
          for (uint64_t c = 0; c < input.channels; c++) {
            // Fill center right
            out.data[out_pos] = (c == 3) ? 1 : 0;
            out_pos++;
          }
        }
      }

      for (uint64_t y = 0; y < block_remainder.y; y++) {
        for (uint64_t x = 0; x < input.size.x; x++) {
          for (uint64_t c = 0; c < input.channels; c++) {
            // Fill bottom
            out.data[out_pos] = (c == 3) ? 1 : 0;
            out_pos++;
          }
        }

        for (uint64_t x = 0; x < block_remainder.x; x++) {
          for (uint64_t c = 0; c < input.channels; c++) {
            // Fill bottom right
            out.data[out_pos] = (c == 3) ? 1 : 0;
            out_pos++;
          }
        }
      }

      

      return out;
    }
  }

  return input;
}
}