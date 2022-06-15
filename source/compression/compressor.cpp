#include <texpress/compression/compressor.hpp>
#include <spdlog/spdlog.h>

#include <compressonator.h>

namespace texpress {
  bool CompressionCallback(float fProgress, CMP_DWORD_PTR pUser1, CMP_DWORD_PTR pUser2) {
    std::printf("\rCompression progress = %3.0f  ", fProgress);
    //spdlog::info("\rCompression progress = {:3.0f}", fProgress);
    return false; // If set true current compression will abort
  }

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

    BlockCompressed encoded{};
    encoded.grid_size = { canonical.size, 1, 1 };                             // images are 2D grids
    encoded.grid_type = DataType::FLOAT32;                                    // HDR = float
    encoded.compression_format = CompressionFormat::BC6H;                     // compression format
    encoded.enc_blocksize= { 4, 4, 1 }; // fixed blocksize for BC6H           // fixed blocksize for all formats except ASTC
    encoded.enc_blocks = encoded.grid_size / glm::ivec4(encoded.enc_blocksize, 1);    // block count in each dimension

    // Compressonator Specifics
    // ========================
    CMP_FORMAT      destFormat = CMP_FORMAT_BC6H;
    CMP_FLOAT       fQuality = 0.05f;

    CMP_Texture srcTexture;
    srcTexture.dwSize = sizeof(srcTexture);
    srcTexture.dwWidth = canonical.size.x;
    srcTexture.dwHeight = canonical.size.y;
    srcTexture.dwPitch = srcTexture.dwWidth;
    srcTexture.format = CMP_FORMAT_RGBA_16F;
    srcTexture.dwDataSize = CMP_CalculateBufferSize(&srcTexture);
    srcTexture.pData = nullptr;

    CMP_Texture dstTexture;
    dstTexture.dwSize = sizeof(dstTexture);
    dstTexture.dwWidth = srcTexture.dwWidth;
    dstTexture.dwHeight = srcTexture.dwHeight;
    dstTexture.dwPitch = srcTexture.dwWidth;
    dstTexture.format = CMP_FORMAT_BC6H;
    dstTexture.dwDataSize = CMP_CalculateBufferSize(&dstTexture);
    dstTexture.pData = nullptr;

    // Step 1: Initialize the Codec: Need to call it only once, repeated calls will return BC_ERROR_LIBRARY_ALREADY_INITIALIZED
    if (CMP_InitializeBCLibrary() != BC_ERROR_NONE) {
      std::printf("BC Codec already initialized!\n");
    }

    // Step 2: Create a BC6H Encoder
    BC6HBlockEncoder* BC6HEncoder;

    // Note we are setting parameters
    CMP_BC6H_BLOCK_PARAMETERS parameters;
    parameters.dwMask = 0xFFFF;   // default
    parameters.fExposure = 0.95;  // default
    parameters.bIsSigned = true;  // signed or unsigned half float (BC6H_UF16 / BC6H_SF16)
    parameters.fQuality;          // not used
    parameters.bUsePatternRec;    // not used

    CMP_CreateBC6HEncoder(parameters, &BC6HEncoder);

    // Convert data to FP16
    uint16_t* fp16_ptr = (uint16_t*)(srcTexture.pData);
    fp16_ptr = new uint16_t[canonical.data.size()];
    if (false && canonical.channels != 3) {
      for (uint64_t pixel = 0; pixel < pixels; pixel++) {
        fp16_ptr[pixel * 4 + 0] = fp16_ieee_from_fp32_value((canonical.channels >= 1) ? canonical.data[pixel * canonical.channels + 0] : 0.f);
        fp16_ptr[pixel * 4 + 1] = fp16_ieee_from_fp32_value((canonical.channels >= 2) ? canonical.data[pixel * canonical.channels + 1] : 0.f);
        fp16_ptr[pixel * 4 + 2] = fp16_ieee_from_fp32_value((canonical.channels >= 3) ? canonical.data[pixel * canonical.channels + 2] : 0.f);
      }

      srcTexture.pData = (uint8_t*)(fp16_ptr);
    }
    else {
      convert_to_f16(canonical.data.size() * sizeof(uint16_t), 0, canonical.data.data(), (uint16_t*)fp16_ptr);

      srcTexture.pData = (uint8_t*)(fp16_ptr);
    }

    // Pointer to source data
    CMP_BYTE* pdata = (CMP_BYTE*)srcTexture.pData;
    const CMP_DWORD dwBlocksX = ((srcTexture.dwWidth + 3) >> 2);
    const CMP_DWORD dwBlocksY = ((srcTexture.dwHeight + 3) >> 2);
    const CMP_DWORD dwBlocksXY = dwBlocksX * dwBlocksY;
    encoded.data_size = dwBlocksXY * 128;
    encoded.data_ptr.resize(encoded.data_size);          // data poointer

    CMP_DWORD dstIndex = 0;    // Destination block index
    CMP_DWORD srcStride = srcTexture.dwWidth * 4;

    dstTexture.pData = new uint8_t[dwBlocksXY * 16]; //blocksize: 128bit (16byte)
    BC_ERROR   cmp_status;

    // Step 4: Process the blocks
    for (CMP_DWORD yBlock = 0; yBlock < dwBlocksY; yBlock++) {

      for (CMP_DWORD xBlock = 0; xBlock < dwBlocksX; xBlock++) {

        // Source block index start base: top left pixel of the 4x4 block
        CMP_DWORD srcBlockIndex = (yBlock * srcStride * 4) + xBlock * 16;

        // Get a input block of data to encode
        CMP_FLOAT blockToEncode[16][4];
        CMP_DWORD srcIndex;
        for (int row = 0; row < 4; row++) {
          srcIndex = srcBlockIndex + (srcStride * row);
          for (int col = 0; col < 4; col++) {
            blockToEncode[row * 4 + col][BC_COMP_RED] = (double)*(pdata + srcIndex++);
            blockToEncode[row * 4 + col][BC_COMP_GREEN] = (double)*(pdata + srcIndex++);
            blockToEncode[row * 4 + col][BC_COMP_BLUE] = (double)*(pdata + srcIndex++);
            blockToEncode[row * 4 + col][BC_COMP_ALPHA] = (double)*(pdata + srcIndex++);
          }
        }

        // Call the block encoder : output is 128 bit compressed data
        cmp_status = CMP_EncodeBC6HBlock(BC6HEncoder, blockToEncode, (encoded.data_ptr.data() + dstIndex));
        if (cmp_status != BC_ERROR_NONE) {
          std::printf(
            "Compression error at block X = %d Block Y = %d \n", xBlock, yBlock);
        }
        dstIndex += 16;

        // Show Progress
        float fProgress = 100.f * (yBlock * dwBlocksX) / dwBlocksXY;

        std::printf("\rCompression progress = %3.0f", fProgress);

      }
    }

    // Step 5 Free up the BC7 Encoder
    CMP_DestroyBC6HEncoder(BC6HEncoder);

    // Step 6 Close the BC Codec
    CMP_ShutdownBCLibrary();

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