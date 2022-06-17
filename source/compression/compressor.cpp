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
    // Parameters
    CMP_BC6H_BLOCK_PARAMETERS parameters;
    parameters.dwMask = 0xFFFF;   // default
    parameters.fExposure = 1.00;  // default
    parameters.bIsSigned = true;  // signed or unsigned half float (BC6H_UF16 / BC6H_SF16)
    parameters.fQuality = 0.05;          // quality
    parameters.bUsePatternRec = false;    // not used

    // Output structure
    auto out = BlockCompressed{};
    out.grid_size = { input.size.x, input.size.y, 1, 1 };
    out.grid_type = DataType::FLOAT32;
    out.enc_blocksize = { 4, 4, 1 };

    // Load the source texture
    CMP_Texture srcTexture;
    srcTexture.dwSize = sizeof(srcTexture);                                     // Size of this structure.
    srcTexture.dwWidth = input.size.x;                                          // Width of the texture.
    srcTexture.dwHeight = input.size.y;                                         // Height of the texture.
    srcTexture.dwPitch = srcTexture.dwWidth * input.channels * sizeof(float);   // Distance to start of next line,
    switch (input.channels) {                                                   // Format of the texture.
    case 1:
      srcTexture.format = CMP_FORMAT_R_32F;
      break;
    case 2:
      srcTexture.format = CMP_FORMAT_RG_32F;
      break;
    case 3:
      srcTexture.format = CMP_FORMAT_RGB_32F;
      break;
    case 4:
      srcTexture.format = CMP_FORMAT_RGBA_32F;
    }
    srcTexture.dwDataSize = CMP_CalculateBufferSize(&srcTexture);   // Size of the current pData texture data
    srcTexture.pData = (CMP_BYTE*)input.data.data();    // Pointer to the texture data to process, this can be the

      // Init dest memory to use for compressed texture
    CMP_Texture destTexture;
    destTexture.dwSize = sizeof(destTexture);
    destTexture.dwWidth = srcTexture.dwWidth;
    destTexture.dwHeight = srcTexture.dwHeight;
    destTexture.dwPitch = std::max(1U, ((destTexture.dwWidth + 3) / 4)) * 4;
    destTexture.format = CMP_FORMAT_BC6H_SF;
    destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
    destTexture.pData = new CMP_BYTE[destTexture.dwDataSize];

    BC_ERROR   cmp_status;
    if (destTexture.format == CMP_FORMAT_BC6H_SF) {

    // Step 1: Initialize the Codec: Need to call it only once, repeated calls will return BC_ERROR_LIBRARY_ALREADY_INITIALIZED
    if (CMP_InitializeBCLibrary() != BC_ERROR_NONE) {
      std::printf("BC Codec already initialized!\n");
    }

    // Step 2: Create a BC6H Encoder
    BC6HBlockEncoder* BC6HEncoder;

    // Note we are setting parameters
    CMP_CreateBC6HEncoder(parameters, &BC6HEncoder);

    // Pointer to source data
    CMP_BYTE* pdata = (CMP_BYTE*)srcTexture.pData;

    const CMP_DWORD dwBlocksX = ((srcTexture.dwWidth + 3) >> 2);
    const CMP_DWORD dwBlocksY = ((srcTexture.dwHeight + 3) >> 2);
    const CMP_DWORD dwBlocksXY = dwBlocksX * dwBlocksY;
    out.enc_blocks = { dwBlocksX, dwBlocksY, 1, 1 };
    out.enc_blocksize = { 4, 4, 1 };

    CMP_DWORD dstIndex = 0;    // Destination block index
    CMP_DWORD srcStride = srcTexture.dwWidth * 4;

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
            blockToEncode[row * 4 + col][BC_COMP_RED] = (CMP_FLOAT)*(pdata + srcIndex++);
            blockToEncode[row * 4 + col][BC_COMP_GREEN] = (CMP_FLOAT)*(pdata + srcIndex++);
            blockToEncode[row * 4 + col][BC_COMP_BLUE] = (CMP_FLOAT)*(pdata + srcIndex++);
            blockToEncode[row * 4 + col][BC_COMP_ALPHA] = (CMP_FLOAT)*(pdata + srcIndex++);
          }
        }

        // Call the block encoder : output is 128 bit compressed data
        cmp_status = CMP_EncodeBC6HBlock(BC6HEncoder, blockToEncode, (destTexture.pData + dstIndex));
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

    // Step 5 Free up the BC6H Encoder
    CMP_DestroyBC6HEncoder(BC6HEncoder);

    // Step 6 Close the BC Codec
    CMP_ShutdownBCLibrary();
    }

    out.data_ptr = std::vector<uint8_t>(destTexture.pData, destTexture.pData + destTexture.dwDataSize);
    out.data_size = destTexture.dwDataSize;
    
    delete destTexture.pData;

    return out;
  }

ldr_image texpress::fit_blocksize(glm::ivec2 blocksize, const ldr_image& input) {
    // If image size is not a multiple of blocksize
    // this will yield the gap of missing pixels to fill
    glm::ivec2 block_remainder = blocksize - (input.size % blocksize);
    block_remainder.x = (block_remainder.x == blocksize.x) ? 0 : block_remainder.x;
    block_remainder.y = (block_remainder.y == blocksize.y) ? 0 : block_remainder.y;

    if ((block_remainder.x != 0) || (block_remainder.y != 0)) {
      ldr_image out{ {}, input.size + block_remainder, input.channels };
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

  return input;
}

hdr_image texpress::fit_blocksize(glm::ivec2 blocksize, const hdr_image& input) {
  // If image size is not a multiple of blocksize
  // this will yield the gap of missing pixels to fill
  glm::ivec2 block_remainder = blocksize - (input.size % blocksize);
  block_remainder.x = (block_remainder.x == blocksize.x) ? 0 : block_remainder.x;
  block_remainder.y = (block_remainder.y == blocksize.y) ? 0 : block_remainder.y;

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

  return input;
}

BlockCompressed Encoder::compress_bc7(const ldr_image& input) {
  // Parameters
  double quality = 0.05;        // Quality set to low
  CMP_BOOL restrictColour = 0;  // Do not restrict colors
  CMP_BOOL restrictAlpha = 0;   // Do not restrict alpha
  CMP_DWORD modeMask = 0xFF;    // Use all BC7 modes
  double performance = 1;       // Performance set to optimal

  // Output structure
  auto out = BlockCompressed{};
  out.grid_size = { input.size.x, input.size.y, 1, 1 };
  out.grid_type = DataType::UINT8;
  out.enc_blocksize = { 4, 4, 1 };

  // Load the source texture
  CMP_Texture srcTexture;
  srcTexture.dwSize = sizeof(srcTexture);                                     // Size of this structure.
  srcTexture.dwWidth = input.size.x;                                          // Width of the texture.
  srcTexture.dwHeight = input.size.y;                                         // Height of the texture.
  srcTexture.dwPitch = srcTexture.dwWidth * input.channels * sizeof(uint8_t); // Distance to start of next line,
  switch (input.channels) {                                                   // Format of the texture.
  case 1:
    srcTexture.format = CMP_FORMAT_R_8;
    break;
  case 2:
    srcTexture.format = CMP_FORMAT_RG_8;
    break;
  case 3:
    srcTexture.format = CMP_FORMAT_RGB_888;
    break;
  case 4:
    srcTexture.format = CMP_FORMAT_RGBA_8888;
  }
  srcTexture.dwDataSize = CMP_CalculateBufferSize(&srcTexture);   // Size of the current pData texture data
  srcTexture.pData = (CMP_BYTE*) input.data.data();    // Pointer to the texture data to process, this can be the

  // Init dest memory to use for compressed texture
  CMP_Texture destTexture;
  destTexture.dwSize = sizeof(destTexture);
  destTexture.dwWidth = srcTexture.dwWidth;
  destTexture.dwHeight = srcTexture.dwHeight;
  destTexture.dwPitch = std::max(1U, ((destTexture.dwWidth + 3) / 4)) * 4;
  destTexture.format = CMP_FORMAT_BC7;
  destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
  destTexture.pData = new CMP_BYTE[destTexture.dwDataSize];

  BC_ERROR   cmp_status;
  if (destTexture.format == CMP_FORMAT_BC7) {

    // Step 1: Initialize the Codec: Need to call it only once, repeated calls will return BC_ERROR_LIBRARY_ALREADY_INITIALIZED
    if (CMP_InitializeBCLibrary() != BC_ERROR_NONE) {
      std::printf("BC Codec already initialized!\n");
    }

    // Step 2: Create a BC7 Encoder
    BC7BlockEncoder* BC7Encoder;

    // Note we are setting quality low for faster encoding in this sample
    CMP_CreateBC7Encoder(
      quality,
      restrictColour,
      restrictAlpha,
      modeMask,
      performance,
      &BC7Encoder);

    // Pointer to source data
    CMP_BYTE* pdata = (CMP_BYTE*)srcTexture.pData;

    const CMP_DWORD dwBlocksX = ((srcTexture.dwWidth + 3) >> 2);
    const CMP_DWORD dwBlocksY = ((srcTexture.dwHeight + 3) >> 2);
    const CMP_DWORD dwBlocksXY = dwBlocksX * dwBlocksY;
    out.enc_blocks = { dwBlocksX, dwBlocksY, 1, 1 };
    out.enc_blocksize = { 4, 4, 1 };

    CMP_DWORD dstIndex = 0;    // Destination block index
    CMP_DWORD srcStride = srcTexture.dwWidth * 4;

    // Step 4: Process the blocks
    for (CMP_DWORD yBlock = 0; yBlock < dwBlocksY; yBlock++) {

      for (CMP_DWORD xBlock = 0; xBlock < dwBlocksX; xBlock++) {

        // Source block index start base: top left pixel of the 4x4 block
        CMP_DWORD srcBlockIndex = (yBlock * srcStride * 4) + xBlock * 16;

        // Get a input block of data to encode
        // Currently the BC7 encoder is using double data formats
        double blockToEncode[16][4];
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
        cmp_status = CMP_EncodeBC7Block(BC7Encoder, blockToEncode, (destTexture.pData + dstIndex));
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
    CMP_DestroyBC7Encoder(BC7Encoder);

    // Step 6 Close the BC Codec
    CMP_ShutdownBCLibrary();
  }

  out.data_ptr = std::vector<uint8_t>(destTexture.pData, destTexture.pData + destTexture.dwDataSize);
  out.data_size = destTexture.dwDataSize;

  delete destTexture.pData;

  return out;
}
}