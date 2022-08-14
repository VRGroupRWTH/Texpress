#include <texpress/helpers/gltfhelper.hpp>
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

namespace texpress {
  Texture load_gltf_binary(const char* path) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, path); // for binary glTF(.glb)

    if (!warn.empty()) {
      printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty()) {
      printf("Err: %s\n", err.c_str());
    }

    if (!ret) {
      printf("Failed to parse glTF\n");
      return {};
    }

    return {};
  }

  bool save_gltf_binary(const char* path, const Texture& input) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    tinygltf::Buffer buffer;
    buffer.data.assign(input.data.data(), input.data.data() + input.bytes());

    tinygltf::BufferView view;
    view.buffer = 0;
    view.byteOffset = 0;
    view.byteLength = buffer.data.size();

    tinygltf::Image img{};
    img.bits = 32;
    img.bufferView = 0;

    return false;
  }
}
/*
#define CGLTF_IMPLEMENTATION
#define CGLTF_WRITE_IMPLEMENTATION
#include <cgltf.h>
#include "cgltf_write.h"
#include <spdlog/spdlog.h>

namespace texpress {
  Texture<uint8_t*> load_gltf(const char* path) {
    cgltf_options options{};
    options.type = cgltf_file_type::cgltf_file_type_invalid;
    options.json_token_count = 0;
    cgltf_data* data = NULL;
  
    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if (result == cgltf_result_success)
    {
      for (auto i = 0; i < data->buffers_count; i++) {
        spdlog::info("Buffer {0} with URI {1} and size {2:d}", data->buffers[i].name, data->buffers[i].uri, data->buffers[i].size);
      }

      for (auto i = 0; i < data->images_count; i++) {
        spdlog::info("Buffer {0} with URI {1}", data->images[i].name, data->images[i].uri);
      }

      cgltf_free(data);
   }
    return {};
  }

  bool save_gltf(const char* path, const Texture<uint8_t*>& input) {
    cgltf_options options{};
    options.type = cgltf_file_type::cgltf_file_type_invalid;
    options.json_token_count = 0;
    cgltf_data* data = NULL;
    data->

    cgltf_image
    
    cgltf_result result = cgltf_write_file(&options, path, data);
    if (result == cgltf_result_success)
    {
      return true;
    }
    return false;
  }
}
*/