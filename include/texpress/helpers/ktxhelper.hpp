#pragma once

#include <texpress/types/texture.hpp>
#include <ktx.h>

namespace texpress {
  template <typename T>
  bool save_ktx(Texture<T> input, const char* path, bool tex_3d = false, bool stack_4d = false) {
    ktxTexture1* texture;
    ktxTextureCreateInfo createInfo;
    KTX_error_code result;
    
    createInfo.glInternalformat = (ktx_uint32_t)input.gl_internalFormat;
    createInfo.baseWidth = input.grid_size.x;
    createInfo.baseHeight = input.grid_size.y;
    createInfo.baseDepth = 1;
    createInfo.numDimensions = 2;
    createInfo.numLevels = 1;
    createInfo.numLayers = 1;
    createInfo.numFaces = 1;
    createInfo.isArray = KTX_FALSE;
    createInfo.generateMipmaps = KTX_FALSE;

    // Use Texture Arrays for saving 3D (4D) data
    if (tex_3d) {
      //createInfo.baseDepth = input.grid_size.z;
      //createInfo.numDimensions = 3;
      createInfo.isArray = KTX_TRUE;
      createInfo.numLayers = input.grid_size.z;
    }
    if (tex_3d && stack_4d) {
      //createInfo.baseDepth *= input.grid_size.w;
      createInfo.numLayers *= input.grid_size.w;
    }

    result = ktxTexture1_Create(&createInfo,
      KTX_TEXTURE_CREATE_ALLOC_STORAGE,
      &texture);

    if (result)
      return false;

    uint64_t slice_elements = input.data_size / createInfo.numLayers / sizeof(T);
    for (uint32_t z = 0; z < createInfo.numLayers; z++) {
      ktx_uint32_t level(0), layer(z), faceSlice(0);
      ktx_size_t srcSize;
      uint8_t* src;

      srcSize = input.data_size / createInfo.numLayers;
      src = (uint8_t*)(input.data.data() + z * slice_elements);

      result = ktxTexture_SetImageFromMemory(ktxTexture(texture), level, layer, faceSlice, src, srcSize);
    }

    /* 
    ktx_size_t offset;
    for (uint64_t i = 0; i < texture->numLayers; i++) {
      result = ktxTexture_GetImageOffset(ktxTexture(texture), 0, i, 0, &offset);
      T* imageData = reinterpret_cast<T*>(ktxTexture_GetData(ktxTexture(texture)) + offset);

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
    
    ktxTexture_WriteToNamedFile(ktxTexture(texture), path);
    ktxTexture_Destroy(ktxTexture(texture));

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
    out.grid_size.z = texture->baseDepth * texture->numLayers;
    out.data_size = ktxTexture_GetDataSize(ktxTexture(texture));
    out.gl_internalFormat = gl::GLenum(texture->glInternalformat);
    out.gl_pixelFormat = gl::GLenum(texture->glFormat);
    out.gl_type = gl::GLenum(texture->glType);
    out.data_channels = gl_colorchannels(out.gl_pixelFormat);

    out.data.reserve(out.data_size / sizeof(T));
    uint64_t tex_size = ktxTexture_GetDataSize(ktxTexture(texture));
    uint64_t slice_elements = tex_size / texture->numLayers / sizeof(T);
    
    for (uint64_t i = 0; i < texture->numLayers; i++) {
      ktx_uint32_t level(0), layer(i), faceSlice(0);
      ktx_size_t offset;

      result = ktxTexture_GetImageOffset(ktxTexture(texture), level, layer, faceSlice, &offset);
      T* imageData = reinterpret_cast<T*>(ktxTexture_GetData(ktxTexture(texture)) + offset);
      out.data.insert(out.data.end(), imageData, imageData + slice_elements);
    }

    /*
    for (uint64_t i = 0; i < texture->numLayers; i++) {
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