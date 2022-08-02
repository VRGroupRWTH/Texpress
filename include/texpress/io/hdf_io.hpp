#pragma once

#include <highfive/H5File.hpp>
#include <highfive/H5DataSet.hpp>
#include <highfive/H5DataSpace.hpp>
#include <glm/glm.hpp>

#include <texpress/io/file_io.hpp>

#include <texpress/export.hpp>

namespace texpress
{
  struct TEXPRESS_EXPORT HDF5Node {
    std::string name;
    HighFive::ObjectType type;
    HDF5Node* parent;
    std::vector<HDF5Node*> children;

    std::string get_path();
    std::string get_string_type();

    bool is_group() const { return HighFive::ObjectType::Group == type; }
    bool is_dataset() const { return HighFive::ObjectType::Dataset == type; }
  };

  struct TEXPRESS_EXPORT HDF5Tree {
    HDF5Node* root;

    HDF5Tree() :
      root(nullptr)
    {}

    bool empty();

    bool insert(std::string path, HighFive::ObjectType h5type);
    std::vector<std::string> list_paths();
    std::vector<HDF5Node*> list_nodes();
    std::vector<HDF5Node*> filter_nodes(HighFive::ObjectType type);
    bool parse(std::string file_path);

  private:
    void list_paths(HDF5Node* node, std::vector<std::string>& paths);
    void list_nodes(HDF5Node* node, std::vector<HDF5Node*>& nodes);
    void filter_nodes(HighFive::ObjectType type, HDF5Node* node, std::vector<HDF5Node*>& nodes);
    bool parse(HighFive::File& file, std::string internal_path);
  };

  class hdf5 {
  public:
    hdf5(const char* path, bool write = false);
    hdf5(const hdf5& that) = delete;
    hdf5(hdf5&& temp) = delete;
    ~hdf5();
    hdf5& operator=(const hdf5& that) = delete;
    hdf5& operator=(hdf5&& temp) = delete;

    template <typename T>
    bool read_datasets(std::vector<const char*> paths, std::vector<uint64_t> offsets, std::vector<uint64_t> strides, std::vector<T>& input) {
      auto dataset = file->getDataSet(paths[0]);
      auto dimensions = dataset.getDimensions();
      std::vector<uint64_t> elements;
      auto elements_flat = 0;
      dataset.getStorageSize();

      // Missing offsets / strides are assumed as regular read operation
      for (int i = 0; i < dimensions.size(); i++) {
        if (offsets.size() < dimensions.size())
          offsets.push_back(0);
        if (strides.size() < dimensions.size())
          strides.push_back(1);
      }

      // Count elements to initialize buffer
      for (int i = 0; i < paths.size(); i++) {
        dataset = file->getDataSet(paths[i]);
        elements.push_back((dataset.getElementCount() - offsets[i]) / strides[i]);
        elements_flat += elements.back();
      }

      input.resize(elements_flat);

      // Fill buffer
      uint64_t buffer_offset = 0;
      for (int i = 0; i < paths.size(); i++) {
        dataset = file->getDataSet(paths[i]);
        dimensions = dataset.getDimensions();

        // Selection parameters specific to current dataset
        std::vector<uint64_t> i_offsets{ offsets[i] };
        std::vector<uint64_t> i_strides{ strides[i] };
        std::vector<uint64_t> i_elements{ (dimensions[0] - i_offsets[0]) / i_strides[0]};

        for (int j = 1; j < dimensions.size(); j++) {
          i_offsets.push_back(0);
          i_strides.push_back(1);
          i_elements.push_back((dimensions[j] - i_offsets[j]) / i_strides[j]);
        }

        dataset.select(i_offsets, i_elements, i_strides).read<T>(input.data() + buffer_offset);
        buffer_offset += elements[i];
      }
      
      return true;
    }

    /*
    template <typename T>
    std::vector<T> read_dataset2(std::vector<const char*> paths, std::vector<uint64_t> offsets, std::vector<uint64_t> strides) {
      auto ds = file->getDataSet(paths[0]);
      auto ds_dims = ds.getDimensions();
      std::vector<uint64_t> ds_elements(paths.size());
      uint64_t elements = 0;

      // Missing offsets / strides are assumed as regular read operation
      for (int i = 0; i < ds_dims.size(); i++) {
        if (offsets.size() < ds_dims.size())
          offsets.push_back(0);
        if (strides.size() < ds_dims.size())
          strides.push_back(1);
      }

      // Count elements to initialize buffer
      for (int i = 0; i < paths.size(); i++) {
        ds = file->getDataSet(paths[i]);
        ds_elements[i] = (ds.getElementCount() - offsets[i]) / strides[i];
        elements += ds_elements[i];
      }

      std::vector<T> out(elements);

      // Fill buffer
      uint64_t out_offset = 0;
      for (int i = 0; i < paths.size(); i++) {
        ds = file->getDataSet(paths[i]);
        
        read(ds, offsets[i], strides[i], reinterpret_cast<uint8_t*>(out.data() + out_offset));
        out_offset += ds_elements[i];
      }

      return out;
    }
    */

    std::vector<uint64_t> dataset_dimensions(const char* dataset);

    private:
      bool read(HighFive::DataSet dataset, uint64_t offset, uint64_t stride, uint8_t* data_ptr);


  private:
    HighFive::File* file;
    const char* filepath;

  };
}
