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

## Installation

### Dependencies
Managed automatically via [vcpkg](https://vcpkg.io/).

### Building:
- Clone the repository
- Run `bootstrap.[sh|bat]`
- The binaries are then available under the `./build` folder.

