#include <texpress/compression/compressor.hpp>
#include <spdlog/spdlog.h>

#include <compressonator.h>

namespace texpress {
  bool CompressionCallback(float fProgress, CMP_DWORD_PTR pUser1, CMP_DWORD_PTR pUser2) {
    std::printf("\rCompression progress = %3.0f  ", fProgress);
    //spdlog::info("\rCompression progress = {:3.0f}", fProgress);
    return false; // If set true current compression will abort
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

  Texture<uint8_t> Encoder::compress_bc6h(const BC6H_options& options, const hdr_image& input) {
    // Output structure
    auto out = Texture<uint8_t>{};
    out.grid_size = { input.size.x, input.size.y, 1, 1 };
    out.grid_glType = TEXPRESS_FLOAT;
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
      spdlog::error("Input must have 4 channels for BC6H!");
      break;
    case 2:
      srcTexture.format = CMP_FORMAT_RG_32F;
      spdlog::error("Input must have 4 channels for BC6H!");
      break;
    case 3:
      srcTexture.format = CMP_FORMAT_ARGB_32F;
      spdlog::error("Input must have 4 channels for BC6H!");
      break;
    case 4:
      srcTexture.format = CMP_FORMAT_ARGB_32F;
    }
    srcTexture.dwDataSize = CMP_CalculateBufferSize(&srcTexture);   // Size of the current pData texture data
    srcTexture.pData = (CMP_BYTE*)input.data.data();

    // Init dest memory to use for compressed texture
    CMP_Texture destTexture;
    destTexture.dwSize = sizeof(destTexture);
    destTexture.dwWidth = srcTexture.dwWidth;
    destTexture.dwHeight = srcTexture.dwHeight;
    destTexture.dwPitch = std::max(1U, ((destTexture.dwWidth + 3) / 4)) * 4;
    if (options.signed_data) {
      destTexture.format = CMP_FORMAT_BC6H_SF;
      out.enc_glformat = TEXPRESS_BC6H_SIGNED;
    }
    else {
      destTexture.format = CMP_FORMAT_BC6H;
      out.enc_glformat = TEXPRESS_BC6H;
    }
    destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
    out.data_size = destTexture.dwDataSize;
    out.data.resize(destTexture.dwDataSize);
    destTexture.pData = out.data.data();

    CMP_CompressOptions cmp_options = { 0 };
    cmp_options.dwSize = sizeof(cmp_options);
    cmp_options.fquality = options.quality;
    cmp_options.dwnumThreads = options.threads;  // Uses auto, else set number of threads from 1..127 max

    CMP_ERROR   cmp_status;
    cmp_status = CMP_ConvertTexture(&srcTexture, &destTexture, &cmp_options, &CompressionCallback);
    if (cmp_status != CMP_OK) {
      std::printf("Compression returned an error %d\n", cmp_status);
      return {};
    }

    return out;
  }

  std::vector<Texture<uint8_t>> Encoder::compress_bc6h(const BC6H_options& options, const std::vector<Texture<float>>& input) {
    // Output structure
    auto out = std::vector<Texture<uint8_t>>(input.size());

    for (int t = 0; t < input[t].grid_size.w; t++) {
      out[t].grid_size = input[t].grid_size;
      out[t].grid_glType = TEXPRESS_FLOAT;
      out[t].enc_blocksize = { 4, 4, 1 };

      if (options.signed_data) {
        out[t].enc_glformat = TEXPRESS_BC6H_SIGNED;
      }
      else {
        out[t].enc_glformat = TEXPRESS_BC6H;
      }

      for (int z = 0; z < input[t].grid_size.z; z++) {
        // Create the source texture
        CMP_Texture srcTexture;
        srcTexture.dwSize = sizeof(srcTexture);                                             // Size of this structure.
        srcTexture.dwWidth = input[t].grid_size.x;                                          // Width of the texture.
        srcTexture.dwHeight = input[t].grid_size.y;                                         // Height of the texture.
        srcTexture.dwPitch = srcTexture.dwWidth * input[t].data_channels * sizeof(float);   // Distance to start of next line,
        switch (input[t].data_channels) {                                                   // Format of the texture.
        case 1:
          srcTexture.format = CMP_FORMAT_R_32F;
          spdlog::error("Input must have 4 channels for BC6H!");
          break;
        case 2:
          srcTexture.format = CMP_FORMAT_RG_32F;
          spdlog::error("Input must have 4 channels for BC6H!");
          break;
        case 3:
          srcTexture.format = CMP_FORMAT_RGB_32F;
          spdlog::error("Input must have 4 channels for BC6H!");
          break;
        case 4:
          srcTexture.format = CMP_FORMAT_ARGB_32F;
        }
        srcTexture.dwDataSize = CMP_CalculateBufferSize(&srcTexture);   // Size of the current pData texture data
        srcTexture.pData = (CMP_BYTE*) (input[t].data.data() + z * srcTexture.dwPitch);

        // Init dest memory to use for compressed texture
        CMP_Texture destTexture;
        destTexture.dwSize = sizeof(destTexture);
        destTexture.dwWidth = srcTexture.dwWidth;
        destTexture.dwHeight = srcTexture.dwHeight;
        destTexture.dwPitch = std::max(1U, ((destTexture.dwWidth + 3) / 4)) * 4;
        if (options.signed_data) {
          destTexture.format = CMP_FORMAT_BC6H_SF;
        }
        else {
          destTexture.format = CMP_FORMAT_BC6H;
        }

        destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
        out[t].data.resize(destTexture.dwDataSize * input[t].grid_size.z);
        destTexture.pData = out[t].data.data() + z * destTexture.dwDataSize;

        CMP_CompressOptions cmp_options = { 0 };
        cmp_options.dwSize = sizeof(cmp_options);
        cmp_options.fquality = options.quality;
        cmp_options.dwnumThreads = options.threads;  // Uses auto, else set number of threads from 1..127 max

        CMP_ERROR   cmp_status;
        cmp_status = CMP_ConvertTexture(&srcTexture, &destTexture, &cmp_options, &CompressionCallback);
        if (cmp_status != CMP_OK) {
          std::printf("Compression returned an error %d\n", cmp_status);
          return {};
        }

        out[t].data_size = destTexture.dwDataSize;
      }
    }

    return out;
  }

  Texture<uint8_t> Encoder::compress_bc6h(const BC6H_options& options, const Texture<float>& input) {
    // Output structure
    auto out = Texture<uint8_t>{};

    for (int t = 0; t < input.grid_size.w; t++) {
      out.grid_size = input.grid_size;
      out.grid_glType = TEXPRESS_FLOAT;
      out.enc_blocksize = { 4, 4, 1 };

      if (options.signed_data) {
        out.enc_glformat = TEXPRESS_BC6H_SIGNED;
      }
      else {
        out.enc_glformat = TEXPRESS_BC6H;
      }

      for (int z = 0; z < input.grid_size.z; z++) {
        // Create the source texture
        CMP_Texture srcTexture;
        srcTexture.dwSize = sizeof(srcTexture);                                     // Size of this structure.
        srcTexture.dwWidth = input.grid_size.x;                                          // Width of the texture.
        srcTexture.dwHeight = input.grid_size.y;                                         // Height of the texture.
        srcTexture.dwPitch = srcTexture.dwWidth * input.data_channels * sizeof(float);   // Distance to start of next line,
        switch (input.data_channels) {                                                   // Format of the texture.
        case 1:
          srcTexture.format = CMP_FORMAT_R_32F;
          spdlog::error("Input must have 4 channels for BC6H!");
          break;
        case 2:
          srcTexture.format = CMP_FORMAT_RG_32F;
          spdlog::error("Input must have 4 channels for BC6H!");
          break;
        case 3:
          srcTexture.format = CMP_FORMAT_RGB_32F;
          spdlog::error("Input must have 4 channels for BC6H!");
          break;
        case 4:
          srcTexture.format = CMP_FORMAT_ARGB_32F;
        }
        srcTexture.dwDataSize = CMP_CalculateBufferSize(&srcTexture);   // Size of the current pData texture data
        srcTexture.pData = (CMP_BYTE*)(input.data.data() + t * z * srcTexture.dwPitch + z * srcTexture.dwPitch);

        // Init dest memory to use for compressed texture
        CMP_Texture destTexture;
        destTexture.dwSize = sizeof(destTexture);
        destTexture.dwWidth = srcTexture.dwWidth;
        destTexture.dwHeight = srcTexture.dwHeight;
        destTexture.dwPitch = std::max(1U, ((destTexture.dwWidth + 3) / 4)) * 4;
        if (options.signed_data) {
          destTexture.format = CMP_FORMAT_BC6H_SF;
        }
        else {
          destTexture.format = CMP_FORMAT_BC6H;
        }

        destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
        out.data.resize(destTexture.dwDataSize * input.grid_size.w * input.grid_size.z);
        destTexture.pData = out.data.data() + t * z * destTexture.dwDataSize + z * destTexture.dwDataSize;

        CMP_CompressOptions cmp_options = { 0 };
        cmp_options.dwSize = sizeof(cmp_options);
        cmp_options.fquality = options.quality;
        cmp_options.dwnumThreads = options.threads;  // Uses auto, else set number of threads from 1..127 max

        CMP_ERROR   cmp_status;
        cmp_status = CMP_ConvertTexture(&srcTexture, &destTexture, &cmp_options, &CompressionCallback);
        if (cmp_status != CMP_OK) {
          std::printf("Compression returned an error %d\n", cmp_status);
          return {};
        }

        out.data_size = destTexture.dwDataSize;
      }
    }

    return out;
  }

  Texture<float> Encoder::decompress_bc6h(const BC6H_options& options, const Texture<uint8_t>& input) {
    // Output structure
    auto out = Texture<float>{};

    for (int t = 0; t < input.grid_size.w; t++) {
      out.grid_size = input.grid_size;
      out.grid_glType = TEXPRESS_FLOAT;

      for (int z = 0; z < input.grid_size.z; z++) {
        // Create the source texture
        CMP_Texture srcTexture;
        srcTexture.dwSize = sizeof(srcTexture);                                          // Size of this structure.
        srcTexture.dwWidth = input.grid_size.x;                                          // Width of the texture.
        srcTexture.dwHeight = input.grid_size.y;                                         // Height of the texture.
        srcTexture.dwPitch = std::max(1U, ((srcTexture.dwWidth + 3) / 4)) * 4;           // Distance to start of next line,
        if (input.enc_glformat == TEXPRESS_BC6H_SIGNED)
          srcTexture.format = CMP_FORMAT_BC6H_SF;
        if (input.enc_glformat == TEXPRESS_BC6H)
          srcTexture.format = CMP_FORMAT_BC6H;
        srcTexture.dwDataSize = CMP_CalculateBufferSize(&srcTexture);   // Size of the current pData texture data
        srcTexture.pData = (CMP_BYTE*)(input.data.data()) + t * z * srcTexture.dwDataSize + z * srcTexture.dwDataSize;

        // Init dest memory to use for compressed texture
        CMP_Texture destTexture;
        destTexture.dwSize = sizeof(destTexture);
        destTexture.dwWidth = srcTexture.dwWidth;
        destTexture.dwHeight = srcTexture.dwHeight;
        destTexture.dwPitch = srcTexture.dwWidth * input.data_channels * sizeof(float);
        destTexture.format = CMP_FORMAT_ARGB_32F;
        destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
        out.data.resize(destTexture.dwDataSize * input.grid_size.w * input.grid_size.z);
        destTexture.pData = (CMP_BYTE*)(out.data.data()) + t * z * destTexture.dwDataSize + z * destTexture.dwDataSize;

        CMP_CompressOptions cmp_options = { 0 };
        cmp_options.dwSize = sizeof(cmp_options);
        cmp_options.fquality = options.quality;
        cmp_options.dwnumThreads = options.threads;  // Uses auto, else set number of threads from 1..127 max

        CMP_ERROR   cmp_status;
        cmp_status = CMP_ConvertTexture(&srcTexture, &destTexture, &cmp_options, &CompressionCallback);
        if (cmp_status != CMP_OK) {
          std::printf("Compression returned an error %d\n", cmp_status);
          return {};
        }

        out.data_size = destTexture.dwDataSize;
      }
    }

    return out;
  }

  Texture<uint8_t> Encoder::compress_bc7(const BC7_options& options, const ldr_image& input) {
  // Output structure
  auto out = Texture<uint8_t>{};
  out.grid_size = { input.size.x, input.size.y, 1, 1 };
  out.grid_glType = TEXPRESS_UINT;
  out.enc_blocksize = { 4, 4, 1 };

  out.enc_glformat = TEXPRESS_BC7;

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
  srcTexture.pData = (CMP_BYTE*)input.data.data();    // Pointer to the texture data to process, this can be the

  // Init dest memory to use for compressed texture
  CMP_Texture destTexture;
  destTexture.dwSize = sizeof(destTexture);
  destTexture.dwWidth = srcTexture.dwWidth;
  destTexture.dwHeight = srcTexture.dwHeight;
  //destTexture.dwPitch = srcTexture.dwWidth;
  destTexture.dwPitch = std::max(1U, ((destTexture.dwWidth + 3) / 4)) * 4;
  destTexture.format = CMP_FORMAT_BC7;
  destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
  out.data_size = destTexture.dwDataSize;
  destTexture.pData = out.data.data();

  CMP_CompressOptions cmp_options = { 0 };
  cmp_options.dwSize = sizeof(cmp_options);
  cmp_options.fquality = options.quality;
  cmp_options.dwnumThreads = options.threads;
  cmp_options.brestrictAlpha = options.restrictAlpha;
  cmp_options.brestrictColour = options.restrictColor;
  cmp_options.dwmodeMask = options.modeMask;

  CMP_ERROR   cmp_status;
  cmp_status = CMP_ConvertTexture(&srcTexture, &destTexture, &cmp_options, &CompressionCallback);
  if (cmp_status != CMP_OK) {
    std::printf("Compression returned an error %d\n", cmp_status);
    return {};
  }

  return out;
}

  Texture<uint8_t> Encoder::compress_bc6h_legacy(const hdr_image& input) {
  // Parameters
  CMP_BC6H_BLOCK_PARAMETERS parameters;
  parameters.dwMask = 0xFFFF;   // default
  parameters.fExposure = 1.00;  // default
  parameters.bIsSigned = true;  // signed or unsigned half float (BC6H_UF16 / BC6H_SF16)
  parameters.fQuality = 0.05;          // quality
  parameters.bUsePatternRec = false;    // not used

  // Output structure
  auto out = Texture<uint8_t>{};
  out.grid_size = { input.size.x, input.size.y, 1, 1 };
  out.grid_glType = TEXPRESS_FLOAT;
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
            blockToEncode[row * 4 + col][BC_COMP_RED] = (CMP_FLOAT) * (pdata + srcIndex++);
            blockToEncode[row * 4 + col][BC_COMP_GREEN] = (CMP_FLOAT) * (pdata + srcIndex++);
            blockToEncode[row * 4 + col][BC_COMP_BLUE] = (CMP_FLOAT) * (pdata + srcIndex++);
            blockToEncode[row * 4 + col][BC_COMP_ALPHA] = (CMP_FLOAT) * (pdata + srcIndex++);
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

  out.data = std::vector<uint8_t>(destTexture.pData, destTexture.pData + destTexture.dwDataSize);
  out.data_size = destTexture.dwDataSize;

  delete destTexture.pData;

  return out;
}

Texture<uint8_t> Encoder::compress_bc7_legacy(const ldr_image& input) {
  // Parameters
  double quality = 0.05;        // Quality set to low
  CMP_BOOL restrictColour = 0;  // Do not restrict colors
  CMP_BOOL restrictAlpha = 0;   // Do not restrict alpha
  CMP_DWORD modeMask = 0xFF;    // Use all BC7 modes
  double performance = 1;       // Performance set to optimal

  // Output structure
  auto out = Texture<uint8_t>{};
  out.grid_size = { input.size.x, input.size.y, 1, 1 };
  out.grid_glType = TEXPRESS_UINT;
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
  srcTexture.pData = (CMP_BYTE*)input.data.data();    // Pointer to the texture data to process, this can be the

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

  out.data = std::vector<uint8_t>(destTexture.pData, destTexture.pData + destTexture.dwDataSize);
  out.data_size = destTexture.dwDataSize;

  delete destTexture.pData;

  return out;
}
}