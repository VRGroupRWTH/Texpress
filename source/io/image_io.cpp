#include <texpress/io/image_io.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace texpress {
  image load_image(const char* path, int requested_channels) {
    image image{};
    uint8_t* image_data = stbi_load(path, &image.size.x, &image.size.y, &image.channels, requested_channels);
    if (requested_channels != 0)
      image.channels = requested_channels;

    if (image_data) {
      image.data.assign(image_data, image_data + image.size.x * image.size.y * image.channels);
      stbi_image_free(image_data);
    }

    return image;
  }

  hdr_image load_image_hdr(const char* path, int requested_channels) {
    hdr_image image{};
    float* image_data = stbi_loadf(path, &image.size.x, &image.size.y, &image.channels, requested_channels);
    if (requested_channels != 0)
      image.channels = requested_channels;

    if (image_data) {
      image.data.assign(image_data, image_data + image.size.x * image.size.y * image.channels);
      stbi_image_free(image_data);
    }

    return image;
  }

  bool save_jpg(const char* path, const uint8_t* data_ptr, glm::ivec2 size, int channels, int quality) {
    if (data_ptr)
      return stbi_write_jpg(path, size.x, size.y, channels, data_ptr, quality);

    return true;
  }

  bool save_png(const char* path, const uint8_t* data_ptr, glm::ivec2 size, int channels) {
    if (data_ptr)
      return stbi_write_png(path, size.x, size.y, channels, data_ptr, size.x * channels * sizeof(uint8_t));

    return true;
  }

  bool save_hdr(const char* path, const float* data_ptr, glm::ivec2 size, int channels) {
    if (data_ptr)
      return stbi_write_hdr(path, size.x, size.y, channels, data_ptr);

    return true;
  }

  bool save_jpg(const char* path, const image& img, int quality) { return save_jpg(path, img.data.data(), img.size, img.channels, quality); }
  bool save_png(const char* path, const image& img) { return save_png(path, img.data.data(), img.size, img.channels); }
  bool save_hdr(const char* path, const hdr_image& img) { return save_hdr(path, img.data.data(), img.size, img.channels); }
}