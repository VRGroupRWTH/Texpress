#pragma once

#include <texpress/types/texture.hpp>
#include <ktx.h>
#include <glm/vec4.hpp>
#include <regex>
#include <texpress/utility/stringtools.hpp>

namespace texpress {
  template <typename T>
  bool save_ktx(Texture<T> input, const char* path, bool as_texture_array = false, bool save_monolithic = false) {
    ktxTexture1* texture;
    ktxTextureCreateInfo createInfo;
    KTX_error_code result;

    glm::ivec4 range;
    range.x = std::max(input.grid_size.x, 1);
    range.y = std::max(input.grid_size.y, 1);
    range.z = std::max(input.grid_size.z, 1);
    range.w = std::max(input.grid_size.w, 1);

    // Automatically a monolithic file if time-dim = 1
    bool monolithic = save_monolithic || (range.w == 1);

    createInfo.glInternalformat = (ktx_uint32_t)input.gl_internalFormat;
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
      uint64_t slice_elements = input.bytes() / (range.w * range.z * sizeof(T));

      for (uint64_t i = 0; i < iterations; i++) {
        for (uint32_t z = 0; z < createInfo.numLayers; z++) {
          ktx_size_t srcSize;
          uint8_t* src;

          srcSize = slice_elements * sizeof(T);
          src = reinterpret_cast<uint8_t*>(input.data.data() + i * createInfo.baseDepth * slice_elements + z * slice_elements);

          result = ktxTexture_SetImageFromMemory(ktxTexture(texture), 0, z, 0, src, srcSize);

          if (result)
            return false;
        }

        std::string filepath = (monolithic) ? str_canonical(path, -1, -1) : str_canonical(path, -1, i);
        ktxTexture_WriteToNamedFile(ktxTexture(texture), filepath.c_str());
      }
    }
    else {
      uint64_t volume_elements = input.bytes() / (range.w * sizeof(T));
      if (monolithic) {
        volume_elements *= range.w;
      }

      for (uint64_t i = 0; i < iterations; i++) {
        texture->pData = reinterpret_cast<uint8_t*>(input.data.data() + i * createInfo.baseDepth * volume_elements);

        std::string filepath = (monolithic) ? str_canonical(path, -1, -1) : str_canonical(path, -1, i);
        ktxTexture_WriteToNamedFile(ktxTexture(texture), filepath.c_str());
      }

      texture->pData = nullptr;
    }

    ktxTexture_Destroy(ktxTexture(texture));

    /* Debug
    ktx_size_t offset;
    for (uint64_t i = 0; i < texture->baseDepth; i++) {
      result = ktxTexture_GetImageOffset(ktxTexture(texture), 0, 0, i, &offset);
      T* imageData = reinterpret_cast<T*>(ktxTexture_GetData(ktxTexture(texture)) + offset);

      if (result)
        spdlog::error("Error Read");

      std::cout << i << ":" << std::endl;
      for (int j = 0; j < 4; j++) {
        std::cout << imageData[j] << ", ";
      }
      std::cout << std::endl;

      for (int j = 4; j < 8; j++) {
        std::cout << imageData[j] << ", ";
      }
      std::cout << std::endl;
      std::cout << std::endl;
    }
    */
    
    return true;
  }

  template <typename T>
  Texture<T> load_ktx(const char* path) {
    ktxTexture1* texture;
    KTX_error_code result;

    result = ktxTexture_CreateFromNamedFile(path,
      KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
      &ktxTexture(texture));

    if (result)
      return { };

    Texture<T> out{};
    out.grid_size.x = texture->baseWidth;
    out.grid_size.y = texture->baseHeight;
    out.grid_size.z = (texture->isArray) ? texture->numLayers : texture->baseDepth;
    out.grid_size.w = (texture->isArray) ? texture->numLayers : texture->baseDepth;
    out.data_size = ktxTexture_GetDataSize(ktxTexture(texture));
    out.gl_internalFormat = gl::GLenum(texture->glInternalformat);
    out.gl_pixelFormat = gl::GLenum(texture->glFormat);
    out.gl_type = gl::GLenum(texture->glType);
    out.data_channels = gl_colorchannels(out.gl_pixelFormat);

    out.data.reserve(out.data_size / sizeof(T));
    uint64_t volume_elements = out.data.capacity();
    uint64_t slice_elements = (texture->isArray) ? volume_elements / texture->numLayers : volume_elements / texture->baseDepth;

    // 2D Array
    if (texture->isArray) {
      for (uint64_t z = 0; z < texture->numLayers; z++) {
        ktx_size_t offset;

        result = ktxTexture_GetImageOffset(ktxTexture(texture), 0, z, 0, &offset);
        T* imageData = reinterpret_cast<T*>(ktxTexture_GetData(ktxTexture(texture)) + offset);
        out.data.insert(out.data.end(), imageData, imageData + slice_elements);
      }
    }
    // 3D Texture
    else {
      T* imageData = reinterpret_cast<T*>(ktxTexture_GetData(ktxTexture(texture)));
      out.data.insert(out.data.end(), imageData, imageData + volume_elements);
    }

    // Debug
    /*
    for (uint64_t i = 0; i < texture->baseDepth * texture->numLayers;  i++) {
      std::cout << i << ":" << std::endl;
      for (int j = 0; j < 4; j++) {
        std::cout << out.data[i * slice_elements + j] << ", ";
      }
      std::cout << std::endl;

      for (int j = 4; j < 8; j++) {
        std::cout << out.data[i * slice_elements + j] << ", ";
      }
      std::cout << std::endl;
      std::cout << std::endl;
    }
    */
    ktxTexture_Destroy(ktxTexture(texture));
    return out;
  }
}