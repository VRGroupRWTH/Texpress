#pragma once
#include <texpress/types/image.hpp>
#include <texpress/types/texture.hpp>
#include <algorithm>

namespace texpress {
  Texture image_to_texture(const image& img) {
    Texture tex{};
    tex.data.assign(img.data.begin(), img.data.end());
    tex.channels = img.channels;
    tex.dimensions.x = std::max(img.size.x, 1);
    tex.dimensions.y = std::max(img.size.y, 1);
    tex.dimensions.z = 1;
    tex.dimensions.w = 1;
    tex.gl_type = img.is_hdr() ? gl::GLenum::GL_FLOAT : gl::GLenum::GL_UNSIGNED_BYTE;
    tex.enc_blocksize = glm::ivec3(0, 0, 0);
    tex.gl_internal = gl_internal(img.channels, img.is_hdr() ? 32 : 8, img.is_hdr());
    tex.gl_format = gl_format(img.channels);

    return tex;
  }

  image texture_to_image(const Texture& tex, int depth = 0, int time = 0, bool normalize = true) {
    image img{};
    img.channels = tex.channels;
    img.size.x = std::max(tex.dimensions.x, 1);
    img.size.y = std::max(tex.dimensions.y, 1);
    img.data.resize(img.size.x * img.size.y * img.channels * tex.bytes_type());
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
}