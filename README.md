# Texpress
BC6 Texture Compression tool for application on vector fields.
Reads 3D and 4D datasets of 3-component floating point vectors and applies [BC6H](https://learn.microsoft.com/en-us/windows/win32/direct3d11/bc6h-format) texture compression.
This allows to use fast native GPU decompression when accessing the data without the need for prior decompression.

The tool previews the source and compressed datasets in image representation and allows to calculate a simple quality estimate.
Datasets can be read from HDF5, RAW and KTX.
They can be saved to RAW, KTX and VTK formats.

### Requirements
- C++17
- [CMake 3.15+](https://cmake.org/)
- [Git](https://git-scm.com/)
- [NVIDIA Texture Tools](https://developer.nvidia.com/nvidia-texture-tools-exporter)

### Features

- Compress/Decompress BC6H
- Read HDF5, RAW and KTX
- Save RAW, KTX and VTK
- Dataset preview
- Quality estimation

### Usage

1. Load file or generate ABC dataset
2. *Optionally:* Normalize dataset to range `[0, 1]`
3. Choose compression quality preset
4. Compress
5. *Optionally:* Measure distance/component error
6. Save

#### Load HDF5 Data

HDF5 data is expected to describe vectors of 3 components.
Each component is loaded seperately.
They can reside in the same dataset in which case a corresponding stride and offset have to be given.

1. Enter path to dataset in text field next to `Select HDF5` button
2. Press `Select HDF5` button
3. View HDF5 File Structure on the right
4. Enter paths for the `[X|Y|Z]` datasets
5. *Optionally:* Choose custom strides/offsets

#### Load RAW Data

RAW data is expected to describe the dataset dimensions `(X, Y, Z, T)` as integers first (in that order) and subsequently the dataset.
For a dataset `datasetname.raw` dimensions can also be given via a seperate file with the naming convention `datasetname_dims.raw` (in this case, the data file **must not** contain dimensions).
The number of channels is calculated automatically based on the filesize and dimensions.

1. Press `Load` button
2. Select data type: `[Source|Normalized|Peaks|Compressed|Decoded|Error]`
3. Give path to dataset
4. Press `Load` button

#### Load KTX Data

KTX data is expected to be given according to the [official specs](https://registry.khronos.org/KTX/specs/1.0/ktxspec.v1.html).

1. Press `Load` button
2. Select data type: `[Source|Normalized|Peaks|Compressed|Decoded|Error]`
3. Give path to dataset
4. Press `Load` button

#### Save Data

Data can be saved in VTK, RAW and KTX format.
Exceptions are the "peaks" data (extrema) generated for normalization/denormalization and the BC6H encoded data.
Peaks are saved as a binary array of floats without any dimension info.
BC6H encoded data cannot be saved as VTK.
Additionally, VTK doesn't support 4D data so the supported datasets generate a seperate VTK file for each timestep containing the respective 3D state of the dataset.

Moreover, data is per default stores in "interleaved" format, i.e., the data is saved as `(x, y, z)` vectors.
In RAW format the data can be saved non-interleaved, where a seperate 4D dataset is saved for each component.

1. Press `Save` button
2. Select data type: `[Source|Normalized|Peaks|Compressed|Decoded|Error]`
3. Give path to dataset
4. Select output type: `[KTX|Raw|VTK]`
5. *Optionally:* If `Raw`, select whether to save the dataset non-interleaved
6. Press `Load` button

#### Errors

The encoder tool can generate rough error estimates of the grid based on the distance between each original and compressed vector or based on the absolute difference of each component.
To do this, the `Source` and `Decoded` datasets need to be populated.
This can be done by either decompressing the encoded representation first or by loading a dataset on disk directly into the `Source` and `Decoded` datasets.

## Installation

### Dependencies
Dependencies are listed in `vcpkg.json` and are managed automatically via [vcpkg](https://vcpkg.io/).

### Building:
- Clone the repository
- Run `bootstrap.[sh|bat]`
- The binaries are then available under the `./build` folder.

