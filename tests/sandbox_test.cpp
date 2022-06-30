#include <catch2/catch_all.hpp>

#include <highfive/H5Easy.hpp>
#include <iostream>
#include <filesystem>
#include <spdlog/spdlog.h>

struct H5Structure {
  std::unique_ptr<H5Structure> parent;
  std::string name;
  HighFive::ObjectType type;
  std::vector<std::unique_ptr<H5Structure>> children;

  bool is_file() { return type == HighFive::ObjectType::File; }
  bool is_Group() { return type == HighFive::ObjectType::Group; }
  bool is_Dataset() { return type == HighFive::ObjectType::Dataset; }
  bool is_Attribute() { return type == HighFive::ObjectType::Attribute; }
};

TEST_CASE("Sandbox testing.", "[texpress::sandbox]")
{
  std::string filepath = std::filesystem::current_path().string() + "\\..\\files\\ctbl3d.nc";
  H5Easy::File file(filepath, H5Easy::File::ReadOnly);

  auto group_root = new HighFive::Group (file.getGroup("/"));

  for (const auto& name : group_root->listObjectNames()) {
    spdlog::info(name);
  }

  spdlog::warn("Attribute Names");
  for (const auto& att : group_root->listAttributeNames()) {
    spdlog::info(att);
  }
}