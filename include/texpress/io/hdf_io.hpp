#pragma once

#include <highfive/H5File.hpp>
#include <highfive/H5DataSet.hpp>
#include <highfive/H5DataSpace.hpp>
#include <glm/glm.hpp>

#include <texpress/export.hpp>

namespace texpress
{







  typedef struct hdf5_handler hdf5_handler;

  hdf5_handler* TEXPRESS_EXPORT hdf5_open(std::string filepath, std::vector<std::string> datasets);

  struct TEXPRESS_EXPORT hdf5_handler
  {
    // hdf5_ variables are HDF5-specific and can't be read (in this implementation). They need to be given.
    HighFive::File           hdf5_file;
    std::vector<std::string> hdf5_datasets;
    std::string              hdf5_filepath;

    /* =========================================================================*/
    /*                             Constructors
    /* =========================================================================*/
    hdf5_handler() = default;
    hdf5_handler(
      std::string              filepath,
      std::vector<std::string> dataset_names
    );

    /* =========================================================================*/
    /*                             Getter / Setter
    /* =========================================================================*/
    // Returns a vector of dynamic size, containing the size of each dimension
    std::vector<std::size_t> get_grid(bool desc_order = false);
    // Returns a vector of size 4, containing the size of each dimension and a fillvalue to fill up to 4 elements
    std::vector<std::size_t> get_grid_fixsize(bool desc_order = false, std::size_t fillvalue = 1);
    // Returns the dimension of the grid
    std::size_t              get_grid_dim();
    // Returns the dimension of the grids elements (1 => scalar, (1, inf) => vector)
    std::size_t              get_vec_len();

    /* =========================================================================*/
    /*                                Methods
    /* =========================================================================*/

    /* Returns size of datatype in bytes.
     * Attention: Will only return the size of the pointer (not the data) if data is variable length data.
     */
    std::size_t get_datatype_bytesize();

    /* Reads HDF5 file into a glm::vector pointer.
     * Will try to read all datasets.
     * It will only read as much datasets as glm::vector can hold; still spare slots in the vector will be ignored.
     * Intended to be used, if you have a data container with glm::vecs.
     */
    template <typename T, int i>
    void read_hdf5(
      glm::vec<i, T>* data_addr
    )
    {
      const auto  shape = get_grid_fixsize();
      const auto  vec_len = get_vec_len();
      // The glm::vec determines the maximum number of read components
      const auto  max_components = (i < vec_len) ? i : vec_len;
      // Grid elements per component
      const auto  grid_elements_scalar = shape[0] * shape[1] * shape[2] * shape[3];

      // HighFive will read the datasets strictly by component. To save the data truly vectorlike we need this buffer.
      T* slice = new T[grid_elements_scalar];

      for (auto c = 0; c < max_components; c++)
      {
        auto ds_name = hdf5_datasets[c];
        auto dataset = hdf5_file.getDataSet(ds_name);

        dataset.read(slice);

        // Copy the components from the buffer to their final position.
        for (std::size_t offset = 0; offset < grid_elements_scalar; offset++)
        {
          (*(data_addr + offset))[c] = *(slice + offset);
        }
      }

      delete[] slice;
    }

    /* Reads HDF5 file partially into a pointer.
     * Will read a single dataset as specified.
     * To achieve a componentlike layout use "stride = 1" and "offset = grid_t * grid_z * grid_y * grid_x * ds_id".
     * E.g.: (u,u,u), ..., (v,v,v), ..., (w,w,w), ...
     */
    template <typename T>
    void read_hdf5(
      T* data_addr,
      std::size_t  data_offset,
      std::size_t  data_stride,
      unsigned int ds_id
    )
    {
      const auto  shape = get_grid_fixsize();
      const auto& grid_x = shape[0];
      const auto& grid_y = shape[1];
      const auto& grid_z = shape[2];
      const auto& grid_t = shape[3];
      // Grid elements per component
      const auto  grid_elements_scalar = grid_t * grid_z * grid_y * grid_x;

      auto dataset = hdf5_file.getDataSet(hdf5_datasets[ds_id]);

      // No Stride
      if (data_stride <= 1)
      {
        dataset.read(data_addr + data_offset);
        return;
      }

      // With Stride
      T* slice = new T[grid_elements_scalar];
      dataset.read(slice);

      for (size_t addr_offset = 0; addr_offset < grid_elements_scalar; addr_offset++)
      {
        (*(data_addr + addr_offset * data_stride + data_offset)) = *(slice + addr_offset);
      }

      delete[] slice;
    }

    // ds_ranges is a vector containing the number of elements for each grid-dimension which should be included
    template <typename T>
    void read_hdf5_subset(
      T* data_addr,
      std::size_t  data_offset,
      std::size_t  data_stride,
      unsigned int ds_id,
      std::vector<std::size_t> ds_ranges,
      std::vector<std::size_t> ds_offsets
    )
    {
      const auto dim = get_grid_dim();
      const auto range_x = (dim >= 1) ? ds_ranges[0] : 1;
      const auto range_y = (dim >= 2) ? ds_ranges[1] : 1;
      const auto range_z = (dim >= 3) ? ds_ranges[2] : 1;
      const auto range_t = (dim >= 4) ? ds_ranges[3] : 1;
      // Range elements per component
      const auto  range_elements_scalar = range_x * range_y * range_z * range_t;

      const auto grid = get_grid_fixsize();
      const auto& grid_x = grid[0];
      const auto& grid_y = grid[1];
      const auto& grid_z = grid[2];
      const auto& grid_t = grid[3];
      // Grid elements per component
      const auto  grid_elements_scalar = grid_x * grid_y * grid_z * grid_t;

      const auto offs_x = (dim >= 1) ? ds_offsets[0] : 0;
      const auto offs_y = (dim >= 2) ? ds_offsets[1] : 0;
      const auto offs_z = (dim >= 3) ? ds_offsets[2] : 0;
      const auto offs_t = (dim >= 4) ? ds_offsets[3] : 0;

      auto dataset = hdf5_file.getDataSet(hdf5_datasets[ds_id]);

      T* slice = new T[grid_elements_scalar];
      dataset.read(slice);

      if (data_stride == 0)
        data_stride = 1;

      /*
      for (size_t addr_offset = 0; addr_offset < range_elements_scalar; addr_offset++)
      {
        (*(data_addr + addr_offset * data_stride + data_offset)) = *(slice + addr_offset);
      }
      */
      size_t initial_offset = offs_t * (grid_z * grid_y * grid_x) + offs_z * (grid_y * grid_x) + offs_y * (grid_x)+offs_x;
      size_t addr_offset = 0;
      for (size_t t = 0; t < range_t; t++)
        for (size_t z = 0; z < range_z; z++)
          for (size_t y = 0; y < range_y; y++)
            for (size_t x = 0; x < range_x; x++)
            {
              (*(data_addr + addr_offset * data_stride + data_offset)) = *(slice + initial_offset + addr_offset);
              addr_offset++;
            }

      delete[] slice;
    }

    /* =========================================================================*/
    /*                                Exceptions
    /* =========================================================================*/
  protected:
    struct GridException : public std::exception {
      const char* what() const throw () {
        return "You can only select datasets with grid-dimension of 1 to 4 and as specified.";
      }
    };
    struct EncodingException : public std::exception {
      const char* what() const throw () {
        return "You selected an encoding that doesn't exactly meet the number of colorchannels. Only L+A supports for 1 or 2 components per pixel.";
      }
    };
    struct VectorEncodingException : public std::exception {
      const char* what() const throw () {
        return "You cannot choose vectorlike encoding when your dataset is scalar and cannot addres more than 1 vectorcomponent.";
      }
    };
  };
}
