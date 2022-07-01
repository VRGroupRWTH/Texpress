#include <catch2/catch_all.hpp>

#include <highfive/H5Easy.hpp>
#include <iostream>
#include <filesystem>
#include <spdlog/spdlog.h>

struct H5Structure {
  H5Structure* parent;
  std::string name;
  HighFive::ObjectType type;
  std::vector<std::unique_ptr<H5Structure>> children;

  H5Structure() :
      parent(nullptr)
    , name("root")
    , type(HighFive::ObjectType::Group)
    , children()
  {}

  H5Structure(H5Structure* h5parent, std::string h5name, HighFive::ObjectType h5type) :
      parent(h5parent)
    , name(h5name)
    , type(h5type)
    , children()
  {}

  bool add_element(std::string path, HighFive::ObjectType h5type) {
    std::vector<std::string> path_elements;
    std::string leaf_element;

    {
      std::stringstream stream(path);
      std::string element;
      while (std::getline(stream, element, '/')) {
        path_elements.push_back(element);
      }
    }

    leaf_element = path_elements.back();
    path_elements.pop_back();

    H5Structure* ptr_current = this;
    for (const auto& element : path_elements) {
      bool element_found = false;
      for (const auto& child : ptr_current->children) {
        if (child->name == element) {
          element_found = true;
          ptr_current = child.get();
          break;
        }
      }

      if (!element_found) {
        spdlog::error("Could not add " + path + " to H5Structure because " + element + " was not found.");
        return false;
      }
    }

    for (const auto& child : ptr_current->children) {
      if (child->name == leaf_element) {
        spdlog::error("Could not add " + path + " to H5Structure because " + leaf_element + " exists already.");
        return false;
      }
    }

    ptr_current->children.push_back(std::make_unique<H5Structure>(this, leaf_element, h5type));
    return true;
  }

  bool is_file() { return type == HighFive::ObjectType::File; }
  bool is_Group() { return type == HighFive::ObjectType::Group; }
  bool is_Dataset() { return type == HighFive::ObjectType::Dataset; }
  bool is_Attribute() { return type == HighFive::ObjectType::Attribute; }

  std::string get_path() {
    if (!parent) {
      // this element is root
      return "/";
    }
    else if (!parent->parent) {
      // if parent is root, don't add trailing slash
      return parent->get_path() + name;
    }
    else {
      return parent->get_path() + "/" + name;
    }
  }

  std::vector<std::string> get_local(HighFive::ObjectType h5type) {
    std::vector<std::string> paths;
    if (type == h5type)
      paths.push_back(get_path());

    for (const auto& child : children) {
      if (child->type == h5type)
        paths.push_back(child->get_path());
    }

    return paths;
  }

  std::vector<std::string> get_global(HighFive::ObjectType h5type) {
    std::vector<std::string> paths;
    if (type == h5type)
      paths.push_back(get_path());

    for (const auto& child : children) {
      auto sub_paths = child->get_global(h5type);
      paths.insert(paths.end(), sub_paths.begin(), sub_paths.end());
    }

    return paths;
  }

  bool parse(std::string file_path) {
    H5Easy::File file(file_path, H5Easy::File::ReadOnly);

    bool parsing = true;
    auto root = HighFive::Group(file.getGroup("/"));
    HighFive::Object* h5object = dynamic_cast<HighFive::Object*>(&root);
    while (parsing) {
      if (h5object->getType() == HighFive::ObjectType::Group) {
        auto group = reinterpret_cast<HighFive::Group*>(h5object);

        for (const auto& name : group->listObjectNames()) {
          add_element(name, group->getObjectType(name));
        }
      }

      if (h5object->getType() == HighFive::ObjectType::Dataset) {
        auto dataset = reinterpret_cast<HighFive::DataSet*>(h5object);
        spdlog::info("dataset");
      }

      if (h5object->getType() == HighFive::ObjectType::Attribute) {
        auto attribute = reinterpret_cast<HighFive::Attribute*>(h5object);
        spdlog::info("attribute");
      }

      if (h5object->getType() == HighFive::ObjectType::DataSpace) {
        auto dataspace = reinterpret_cast<HighFive::DataSpace*>(h5object);
        spdlog::info("dataspace");
      }
      
      parsing = false;
    }

    return true;
  }
};

TEST_CASE("Sandbox testing.", "[texpress::sandbox]")
{
  std::string filepath = std::filesystem::current_path().string() + "\\..\\files\\ctbl3d.nc";
  H5Easy::File file(filepath, H5Easy::File::ReadOnly);

  auto h5struct = H5Structure();
  h5struct.parse(std::filesystem::current_path().string() + "\\..\\files\\ctbl3d.nc");

  auto groups = h5struct.get_global(HighFive::ObjectType::Group);
  auto datasets = h5struct.get_global(HighFive::ObjectType::Dataset);

  for (const auto& group : groups) {
    spdlog::info("Group: " + group);
  }

  for (const auto& dataset : datasets) {
    spdlog::info("Dataset: " + dataset);
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