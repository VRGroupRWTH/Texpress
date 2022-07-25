#include <catch2/catch_all.hpp>

#include <iostream>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <texpress/api.hpp>

TEST_CASE("Sandbox testing.", "[texpress::sandbox]")
{
  auto img = texpress::load_image_hdr((std::filesystem::current_path().string() + "\\..\\files\\noise.jpg").c_str());

  if (true) {
    auto tex = texpress::image_to_texture<float>(img);

    for (int i = 0; i < 1; i++) {
      //tex.data.insert(tex.data.end(), tex.data.begin(), tex.data.end());
      for (int p = 0; p < tex.grid_size.x * tex.grid_size.y * tex.data_channels; p++) {
        tex.data.push_back(1.0f);
      }
      //tex.grid_size.z *= 2;
      //tex.data_size *= 2;
      tex.data_size += tex.data_size / tex.grid_size.z;
      tex.grid_size.z += 1;
    }

    //texpress::save_exr("test_exr", tex);
    //auto test = texpress::load_exr_tex("test_exr_0_0.exr");
    //test = texpress::load_exr_tex("test_exr_1_0.exr");
    if (!texpress::save_ktx(tex, "3d_ktx", true, false)) {
      spdlog::error("Could not save");
    }
  }
  else {
    texpress::save_exr("test_exr.exr", img);
    auto test = texpress::load_exr("test_exr.exr");

  }
}