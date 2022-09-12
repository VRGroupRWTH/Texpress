#pragma once

#include <texpress/types/texture.hpp>
#include <glm/vec4.hpp>

namespace texpress {
  uint64_t ktx_size(const char* path);

  bool save_ktx(const uint8_t* data_ptr, const char* path, const glm::ivec4& dimensions, gl::GLenum gl_internal_format, uint64_t size, bool as_texture_array = false, bool save_monolithic = false);
  bool save_ktx2(const uint8_t* data_ptr, const char* path, const glm::ivec4& dimensions, gl::GLenum gl_internal_format, uint64_t size, bool as_texture_array = false, bool save_monolithic = false);
  bool save_ktx2(const uint8_t* data_ptr, const char* path, const glm::ivec4& dimensions, VkFormat vk_format, uint32_t channels, uint32_t type_size, uint64_t size, bool as_texture_array = false, bool save_monolithic = false);
  void load_ktx(const char* path, uint8_t* data_ptr, uint8_t& channels, glm::ivec4& dimensions, gl::GLenum& gl_internal_format, gl::GLenum& gl_format, gl::GLenum& gl_type);

  bool save_ktx(const Texture& input, const char* path, bool as_texture_array = false, bool save_monolithic = false);
  bool save_ktx2(const Texture& input, const char* path, bool as_texture_array = false, bool save_monolithic = false);

  void load_ktx(const char* path, Texture& tex);
}