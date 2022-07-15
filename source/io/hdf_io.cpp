#include <texpress/io/hdf_io.hpp>
#include <spdlog/spdlog.h>

namespace texpress
{
    std::string HDF5Node::get_path() {
      if (!parent) {
        return "/" + name;
      }

      auto subpath = parent->get_path();
      if (subpath.back() == '/')
        return parent->get_path() + name;
      else
        return parent->get_path() + "/" + name;
    }

    std::string HDF5Node::get_string_type() {
      switch (type) {
      case HighFive::ObjectType::Group:
        return "Group";
      case HighFive::ObjectType::Dataset:
        return "Dataset";
      case HighFive::ObjectType::DataSpace:
        return "DataSpace";
      case HighFive::ObjectType::Attribute:
        return "Attribute";
      case HighFive::ObjectType::UserDataType:
        return "UserDataType";
      }

      return "Other";
    }

    bool HDF5Tree::empty() {
      return !root;
    }

    bool HDF5Tree::insert(std::string path, HighFive::ObjectType h5type) {
      std::vector<std::string> path_elements;
      std::string leaf_element;

      {
        std::stringstream stream(path);
        std::string element;
        while (std::getline(stream, element, '/')) {
          if (!element.empty())
            path_elements.push_back(element);
        }
      }

      leaf_element = path_elements.back();
      path_elements.pop_back();

      HDF5Node* ptr_current = root;
      for (const auto& element : path_elements) {
        bool element_found = false;
        for (const auto& child : ptr_current->children) {
          if (child->name == element) {
            element_found = true;
            ptr_current = child;
            break;
          }
        }

        if (!element_found) {
          spdlog::error("Could not add '" + path + "' to HDF5Tree because '" + element + "' was not found.");
          return false;
        }
      }

      for (const auto& child : ptr_current->children) {
        if (child->name == leaf_element) {
          spdlog::error("Could not add '" + path + "' to HDF5Tree because '" + leaf_element + "' exists already.");
          return false;
        }
      }

      ptr_current->children.push_back(new HDF5Node{ leaf_element, h5type, ptr_current, {} });
      return true;
    }

    std::vector<std::string> HDF5Tree::list_paths() {
      if (!root) {
        return { };
      }

      std::vector<std::string> paths;
      paths.push_back(root->get_path());
      for (const auto& child : root->children) {
        list_paths(child, paths);
      }

      return paths;
    }

    std::vector<HDF5Node*> HDF5Tree::list_nodes() {
      if (!root) {
        return { };
      }

      std::vector<HDF5Node*> nodes;
      nodes.push_back(root);
      for (const auto& child : root->children) {
        list_nodes(child, nodes);
      }

      return nodes;
    }

    std::vector<HDF5Node*> HDF5Tree::filter_nodes(HighFive::ObjectType type) {
      if (empty()) {
        return { };
      }

      std::vector<HDF5Node*> nodes;
      if (root->type == type)
        nodes.push_back(root);
      for (const auto& child : root->children) {
        filter_nodes(type, child, nodes);
      }

      return nodes;
    }

    bool HDF5Tree::parse(std::string file_path) {
      HighFive::File file(file_path, HighFive::File::ReadOnly);

      root = new HDF5Node{ "", HighFive::ObjectType::Group, nullptr, {} };

      return parse(file, "/");
    }

    void HDF5Tree::list_paths(HDF5Node* node, std::vector<std::string>& paths) {
      paths.push_back(node->get_path());

      for (const auto& child : node->children) {
        list_paths(child, paths);
      }
    }

    void HDF5Tree::list_nodes(HDF5Node* node, std::vector<HDF5Node*>& nodes) {
      nodes.push_back(node);

      for (const auto& child : node->children) {
        list_nodes(child, nodes);
      }
    }

    void HDF5Tree::filter_nodes(HighFive::ObjectType type, HDF5Node* node, std::vector<HDF5Node*>& nodes) {
      if (node->type == type)
        nodes.push_back(node);

      for (const auto& child : node->children) {
        filter_nodes(type, child, nodes);
      }
    }

    bool HDF5Tree::parse(HighFive::File& file, std::string internal_path) {
      bool parsing = true;
      bool ok = true;
      auto group = file.getGroup(internal_path);
      for (const auto& name : group.listObjectNames()) {
        auto type = group.getObjectType(name);
        auto subpath = internal_path;
        if (subpath.back() == '/')
          subpath += name;
        else
          subpath += "/" + name;
        insert(subpath, type);
        if (type == HighFive::ObjectType::Group)
          ok = parse(file, subpath);
      }
      return ok;
    }


  hdf5::hdf5(const char* path, bool write) {
    unsigned int flags = HighFive::File::ReadOnly + write;
    file = new HighFive::File(path, flags);
    filepath = path;
  }

  hdf5::~hdf5() {
    delete file;
  }

  /* =========================================================================*/
  /*                             Constructors
  /* =========================================================================*/
  hdf5_handler::hdf5_handler(
    std::string              filepath,
    std::vector<std::string> dataset_names
  ) : hdf5_file{ filepath, HighFive::File::ReadOnly }
    , hdf5_datasets{ dataset_names }
  {
    // no-op
  }


  /* =========================================================================*/
  /*                             Getter / Setter
  /* =========================================================================*/
  std::size_t hdf5_handler::get_datatype_bytesize()
  {
    HighFive::DataSet  dataset = hdf5_file.getDataSet(hdf5_datasets[0]);
    HighFive::DataType datatype = dataset.getDataType();

    return datatype.getSize();
  }

  std::vector<std::size_t> hdf5_handler::get_grid(bool desc_order)
  {
    HighFive::DataSet        dataset = hdf5_file.getDataSet(hdf5_datasets[0]);
    std::vector<std::size_t> grid = dataset.getDimensions();
    std::uint8_t             grid_dim = grid.size();

    std::size_t  grid_t = (grid_dim == 4) ? grid[grid_dim - 4] : 0;
    std::size_t  grid_z = (grid_dim >= 3) ? grid[grid_dim - 3] : 0;
    std::size_t  grid_y = (grid_dim >= 2) ? grid[grid_dim - 2] : 0;
    std::size_t  grid_x = (grid_dim >= 1) ? grid[grid_dim - 1] : 0;

    switch (grid_dim)
    {
    case 1:
      return { grid_x };

    case 2:
      if (desc_order)
        return { grid_y, grid_x };
      else
        return { grid_x, grid_y };

    case 3:
      if (desc_order)
        return { grid_z, grid_y, grid_x };
      else
        return { grid_x, grid_y, grid_z };

    case 4:
      if (desc_order)
        return { grid_t, grid_z, grid_y, grid_x };
      else
        return { grid_x, grid_y, grid_z, grid_t };
    }

    return { };
  }

  std::vector<std::size_t> hdf5_handler::get_grid_fixsize(bool desc_order, std::size_t fillvalue)
  {
    HighFive::DataSet        dataset = hdf5_file.getDataSet(hdf5_datasets[0]);
    std::vector<std::size_t> grid = dataset.getDimensions();
    std::uint8_t             grid_dim = grid.size();

    std::size_t  grid_t = (grid_dim == 4) ? grid[grid_dim - 4] : 0;
    std::size_t  grid_z = (grid_dim >= 3) ? grid[grid_dim - 3] : 0;
    std::size_t  grid_y = (grid_dim >= 2) ? grid[grid_dim - 2] : 0;
    std::size_t  grid_x = (grid_dim >= 1) ? grid[grid_dim - 1] : 0;

    std::size_t& fv = fillvalue;
    switch (grid_dim)
    {
    case 1:
      if (desc_order)
        return { fv, fv, fv, grid_x };

      else
        return { grid_x, fv, fv, fv };

    case 2:
      if (desc_order)
        return { fv, fv, grid_y, grid_x };
      else
        return { grid_x, grid_y, fv, fv };

    case 3:
      if (desc_order)
        return { fv, grid_z, grid_y, grid_x };
      else
        return { grid_x, grid_y, grid_z, fv };

    case 4:
      if (desc_order)
        return { grid_t, grid_z, grid_y, grid_x };
      else
        return { grid_x, grid_y, grid_z, grid_t };
    }

    return { };
  }

  std::size_t hdf5_handler::get_grid_dim()
  {
    HighFive::DataSet        dataset = hdf5_file.getDataSet(hdf5_datasets[0]);
    std::vector<std::size_t> grid = dataset.getDimensions();

    return grid.size();
  }

  std::size_t hdf5_handler::get_vec_len()
  {
    return hdf5_datasets.size();
  }
}