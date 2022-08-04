#pragma once

#include <texpress/types/texture.hpp>
#include <ktx.h>
#include <glm/vec4.hpp>
#include <regex>
#include <texpress/utility/stringtools.hpp>

namespace texpress {
  bool save_ktx(const uint8_t* data_ptr, const char* path, const glm::ivec4& dimensions, gl::GLenum gl_internal_format, uint64_t size, bool as_texture_array = false, bool save_monolithic = false);

  template <typename T>
  bool save_ktx(const Texture<T>& input, const char* path, bool as_texture_array = false, bool save_monolithic = false) {
    return save_ktx(reinterpret_cast<const uint8_t*>(input.data.data()), path, input.dimensions, input.gl_internal, input.bytes(), as_texture_array, save_monolithic);
  }

  /*
  template <typename T>
  bool save_ktx(Texture<T> input, const char* path, bool as_texture_array = false, bool save_monolithic = false) {
    ktxTexture1* texture;
    ktxTextureCreateInfo createInfo;
    KTX_error_code result;

    ktxTexture2;

    glm::ivec4 range;
    range.x = std::max(input.dimensions.x, 1);
    range.y = std::max(input.dimensions.y, 1);
    range.z = std::max(input.dimensions.z, 1);
    range.w = std::max(input.dimensions.w, 1);

    // Automatically a monolithic file if time-dim = 1
    bool monolithic = save_monolithic || (range.w == 1);

    createInfo.glInternalformat = (ktx_uint32_t)input.gl_internal;
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
    
    return true;
  }
  */

  template <typename T>
  void load_ktx(const char* path, Texture<T>& tex) {
    ktxTexture1* texture;
    KTX_error_code result;

    result = ktxTexture_CreateFromNamedFile(path,
      KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
      &ktxTexture(texture));

    if (result)
      return;

    tex.data.clear();
    tex.dimensions.x = texture->baseWidth;
    tex.dimensions.y = texture->baseHeight;
    tex.dimensions.z = (texture->isArray) ? texture->numLayers : texture->baseDepth;
    tex.dimensions.w = 1;
    tex.gl_internal = gl::GLenum(texture->glInternalformat);
    tex.gl_format = gl::GLenum(texture->glFormat);
    tex.gl_type = gl::GLenum(texture->glType);
    tex.channels = gl_channels(tex.gl_format);

    char* pValue;
    uint32_t valueLen;
    if (KTX_SUCCESS == ktxHashList_FindValue(&texture->kvDataHead,
      "Dimensions",
      &valueLen, (void**)&pValue))
    {
      auto dimensions = *reinterpret_cast<glm::ivec4*>(pValue);
      tex.dimensions.z = dimensions.z;
      tex.dimensions.w = dimensions.w;
    }

    tex.data.reserve(ktxTexture_GetDataSize(ktxTexture(texture)) / sizeof(T));
    uint64_t volume_elements = tex.data.capacity();
    uint64_t slice_elements = (texture->isArray) ? volume_elements / texture->numLayers : volume_elements / texture->baseDepth;

    // 2D Array
    if (texture->isArray) {
      for (uint64_t z = 0; z < texture->numLayers; z++) {
        ktx_size_t offset;

        result = ktxTexture_GetImageOffset(ktxTexture(texture), 0, z, 0, &offset);
        T* imageData = reinterpret_cast<T*>(ktxTexture_GetData(ktxTexture(texture)) + offset);
        tex.data.insert(tex.data.end(), imageData, imageData + slice_elements);
      }
    }
    // 3D Texture
    else {
      T* imageData = reinterpret_cast<T*>(ktxTexture_GetData(ktxTexture(texture)));
      tex.data.insert(tex.data.end(), imageData, imageData + volume_elements);
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
  }
}