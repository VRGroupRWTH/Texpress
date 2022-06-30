#include <texpress/io/hdf_io.hpp>

namespace texpress
{
  hdf5_handler* hdf5_open(std::string filepath, std::vector<std::string> datasets) {
    return nullptr;
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