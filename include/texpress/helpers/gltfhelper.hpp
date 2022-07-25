#pragma once
#include <texpress/types/texture.hpp>

namespace texpress {
  Texture<uint8_t*> load_gltf_binary(const char* path);
  bool save_gltf_binary(const char* path, const Texture<float>& input);
}