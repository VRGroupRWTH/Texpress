#pragma once
#include <texpress/helpers/ktxhelper.hpp>
#include <string>

namespace texpress {
  bool save_ktx(const uint8_t* data_ptr, const char* path, const glm::ivec4& dimensions, gl::GLenum gl_internal_format, uint64_t size, bool as_texture_array, bool save_monolithic) {
    ktxTexture1* texture;
    ktxTextureCreateInfo createInfo;
    KTX_error_code result;

    glm::ivec4 range;
    range.x = std::max(dimensions.x, 1);
    range.y = std::max(dimensions.y, 1);
    range.z = std::max(dimensions.z, 1);
    range.w = std::max(dimensions.w, 1);

    uint32_t channels = gl_channels(gl::GLenum(gl_internal_format));
    uint32_t type_size = gl_compressed(gl::GLenum(gl_internal_format)) ? 1 : size / (range.x * range.y * range.z * range.w * channels);

    // Automatically a monolithic file if time-dim = 1
    bool monolithic = save_monolithic || (range.w == 1);

    createInfo.glInternalformat = (ktx_uint32_t)gl_internal_format;
    createInfo.baseWidth = range.x;
    createInfo.baseHeight = range.y;
    createInfo.baseDepth = (monolithic) ? range.z * range.w : range.z;
    createInfo.numDimensions = 3;
    createInfo.numLevels = 1;
    createInfo.numLayers = 1;
    createInfo.numFaces = 1;
    createInfo.isArray = KTX_FALSE;
    createInfo.generateMipmaps = KTX_FALSE;

    if (as_texture_array) {
      createInfo.isArray = KTX_TRUE;
      createInfo.baseDepth = 1;
      createInfo.numDimensions = 2;
      createInfo.numLayers = (monolithic) ? range.z * range.w : range.z;
    }

    result = ktxTexture1_Create(&createInfo,
      KTX_TEXTURE_CREATE_ALLOC_STORAGE,
      &texture);

    if (result)
      return false;

    // Each iteration produces a seperate file
    uint64_t iterations = (monolithic) ? 1 : range.w;

    if (as_texture_array) {
      uint64_t slice_elements = size / (range.w * range.z * type_size);

      for (uint64_t i = 0; i < iterations; i++) {
        for (uint32_t z = 0; z < createInfo.numLayers; z++) {
          ktx_size_t srcSize = slice_elements * type_size;;
          uint8_t* src = (uint8_t*) data_ptr + i * createInfo.baseDepth * srcSize + z * srcSize;

          result = ktxTexture_SetImageFromMemory(ktxTexture(texture), 0, z, 0, src, srcSize);

          if (result)
            return false;
        }

        std::string filepath = (monolithic) ? str_canonical(path, -1, -1) : str_canonical(path, -1, i);
        ktxTexture_WriteToNamedFile(ktxTexture(texture), filepath.c_str());
      }
    }
    else {
      uint64_t volume_elements = size / (range.w * type_size);
      if (monolithic) {
        volume_elements *= range.w;
      }

      for (uint64_t i = 0; i < iterations; i++) {
        ktx_size_t srcSize = volume_elements * type_size;;
        texture->pData = (uint8_t*) data_ptr + i * createInfo.baseDepth * srcSize;

        std::string filepath = (monolithic) ? str_canonical(path, -1, -1) : str_canonical(path, -1, i);
        ktxTexture_WriteToNamedFile(ktxTexture(texture), filepath.c_str());
      }

      texture->pData = nullptr;
    }

    ktxTexture_Destroy(ktxTexture(texture));

    return true;
  }
}