#include <catch2/catch_all.hpp>

#include <highfive/H5Easy.hpp>
#include <iostream>
#include <filesystem>
#include <spdlog/spdlog.h>

TEST_CASE("Sandbox testing.", "[texpress::sandbox]")
{
  std::string filepath = std::filesystem::current_path().string() + "\\..\\files\\test.HDF5";
  H5Easy::File file(filepath, H5Easy::File::ReadOnly);

  auto h5struct = texpress::HDF5Tree();
  h5struct.parse(filepath);
  
  auto groups = h5struct.list_paths();
  auto nodes = h5struct.list_nodes();

  for (const auto& group : groups) {
    spdlog::info(group);
  }

  /*
  bool parsing = true;
  while (parsing) {
    file.getNumberObjects();

  }
  auto group_root = new HighFive::Group (file.getGroup("/"));

  for (const auto& name : group_root->listObjectNames()) {
    spdlog::info(name);
  }

  spdlog::warn("Attribute Names");
  for (const auto& att : group_root->listAttributeNames()) {
    spdlog::info(att);
  }*/
}