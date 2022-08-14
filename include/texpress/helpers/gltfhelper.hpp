#pragma once
#include <texpress/types/texture.hpp>

namespace texpress {
  Texture load_gltf_binary(const char* path);
  bool save_gltf_binary(const char* path, const Texture& input);
}