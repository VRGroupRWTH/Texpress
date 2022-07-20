#pragma once
#include <texpress/types/texture.hpp>
#include <texpress/types/image.hpp>

namespace texpress {
  image load_image(const char* path, int requested_channels = 0);
  hdr_image load_image_hdr(const char* path, int requested_channels = 0);

  bool save_jpg(const char* path, const image& img, int quality = 100);
  bool save_png(const char* path, const image& img);
  bool save_hdr(const char* path, const hdr_image& img);

  bool save_jpg(const char* path, const uint8_t* data_ptr, glm::ivec2 size, int channels, int quality = 100);
  bool save_png(const char* path, const uint8_t* data_ptr, glm::ivec2 size, int channels);
  bool save_hdr(const char* path, const float* data_ptr, glm::ivec2 size, int channels);
}