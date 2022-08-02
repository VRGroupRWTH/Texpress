#pragma once
#include <texpress/types/image.hpp>
#include <texpress/types/texture.hpp>
#include <algorithm>

namespace texpress {
  template <typename T>
  Texture<T> image_to_texture(image_ldr img) {
    Texture<T> tex{};
    tex.data.assign(img.data.begin(), img.data.end());
    tex.data_size = img.data.size() * sizeof(T);
    tex.channels = img.channels;
    tex.dimensions.x = std::max(img.size.x, 1);
    tex.dimensions.y = std::max(img.size.y, 1);
    tex.dimensions.z = 1;
    tex.dimensions.w = 1;
    tex.gl_type = gl::GLenum::GL_UNSIGNED_BYTE;
    tex.enc_blocksize = glm::ivec3(0, 0, 0);
    tex.gl_internal = gl_internal(img.channels, 8, false);
    tex.gl_format = gl_format(img.channels);

    return tex;
  }

  template <typename T>
  Texture<T> image_to_texture(image_hdr img) {
    Texture<T> tex{};
    tex.data.assign(img.data.begin(), img.data.end());
    tex.data_size = img.data.size() * sizeof(T);
    tex.channels = img.channels;
    tex.dimensions.x = std::max(img.size.x, 1);
    tex.dimensions.y = std::max(img.size.y, 1);
    tex.dimensions.z = 1;
    tex.dimensions.w = 1;
    tex.gl_type = gl::GLenum::GL_FLOAT;
    tex.enc_blocksize = glm::ivec3(0, 0, 0);
    tex.gl_internal = gl_internal(img.channels, 32, true);
    tex.gl_format = gl_format(img.channels);


    return tex;
  }

  template <typename T>
  image_ldr texture_to_image(Texture<T> tex, int depth = 0, int time = 0, bool normalize = true) {
    image_ldr img{};
    img.channels = tex.channels;
    img.size.x = std::max(tex.dimensions.x, 1);
    img.size.y = std::max(tex.dimensions.y, 1);
    img.data.resize(img.size.x * img.size.y * img.channels);
    uint64_t offset = (time * depth + depth) * img.size.x * img.size.y;

    if (normalize) {
      auto max = std::max_element(tex.data.begin() + offset, tex.data.begin() + offset + img.data.size())[0];
      auto min = std::min_element(tex.data.begin() + offset, tex.data.begin() + offset + img.data.size())[0];
      for (int i = 0; i < img.data.size(); i++) {
        float normalized = (tex.data[offset + i] - min) / (max - min);
        img.data[i] = static_cast<uint8_t>(normalized * 255);
      }
    }
    else {
      for (int i = 0; i < img.data.size(); i++) {
        img.data[i] = static_cast<uint8_t>(tex.data[i + offset]);
      }
    }

    return img;
  }

  template <typename T>
  image_hdr texture_to_image_hdr(Texture<T> tex, int depth = 0, int time = 0, bool normalize = true) {
    image_hdr img{};
    img.channels = tex.channels;
    img.size.x = std::max(tex.dimensions.x, 1);
    img.size.y = std::max(tex.dimensions.y, 1);
    img.data.resize(img.size.x * img.size.y * img.channels);
    uint64_t offset = (time * depth + depth) * img.size.x * img.size.y;

    if (normalize) {
      auto max = std::max_element(tex.data.begin() + offset, tex.data.begin() + offset + img.data.size())[0];
      auto min = std::min_element(tex.data.begin() + offset, tex.data.begin() + offset + img.data.size())[0];
      for (int i = 0; i < img.data.size(); i++) {
        float normalized = (tex.data[offset + i] - min) / (max - min);
        img.data[i] = static_cast<float>(normalized);
      }
    }
    else {
      for (int i = 0; i < img.data.size(); i++) {
        img.data[i] = static_cast<float>(tex.data[i + offset]);
      }
    }

    return img;
  }
}