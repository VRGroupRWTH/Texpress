#pragma once
#include <texpress/types/texture.hpp>
#include <texpress/types/image.hpp>
#include <string>

namespace texpress {
  image load_image(const char* path, int requested_channels = 0);
  image load_image_hdr(const char* path, int requested_channels = 0);
  image load_exr(const char* path);
  Texture load_exr_tex(const char* path);

  bool save_jpg(const char* path, const image& img, int quality = 100);
  bool save_png(const char* path, const image& img);
  bool save_hdr(const char* path, const image& img);
  bool save_exr_bgr(const char* path, const image& img);
  bool save_exr(const char* path, const image& img, const std::string order = "BGRA");
  bool save_exr(const char* path, const Texture& img, const std::string order = "BGRA");

  bool save_jpg(const char* path, const uint8_t* data_ptr, glm::ivec2 size, int channels, int quality = 100);
  bool save_png(const char* path, const uint8_t* data_ptr, glm::ivec2 size, int channels);
  bool save_hdr(const char* path, const uint8_t* data_ptr, glm::ivec2 size, int channels);
  bool save_exr_bgr(const char* path, const uint8_t* data_ptr, glm::ivec2 size, int channels);
  bool save_exr(const char* path, const uint8_t* data_ptr, glm::ivec2 size, int channels, const std::string order = "BGRA");
  bool save_exr(const char* path, const uint8_t* data_ptr, glm::ivec4 size, int channels, const std::string order = "BGRA");
}