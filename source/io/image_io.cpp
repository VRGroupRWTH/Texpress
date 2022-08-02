#include <texpress/io/image_io.hpp>
#include <string>
#include <texpress/utility/stringtools.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <tinyexr.h>

namespace texpress {
  image_ldr load_image(const char* path, int requested_channels) {
    image_ldr image_ldr{};
    uint8_t* image_data = stbi_load(path, &image_ldr.size.x, &image_ldr.size.y, &image_ldr.channels, requested_channels);
    if (requested_channels != 0)
      image_ldr.channels = requested_channels;

    if (image_data) {
      image_ldr.data.assign(image_data, image_data + image_ldr.size.x * image_ldr.size.y * image_ldr.channels);
      stbi_image_free(image_data);
    }

    return image_ldr;
  }

  image_hdr load_image_hdr(const char* path, int requested_channels) {
    image_hdr image_ldr{};
    float* image_data = stbi_loadf(path, &image_ldr.size.x, &image_ldr.size.y, &image_ldr.channels, requested_channels);
    if (requested_channels != 0)
      image_ldr.channels = requested_channels;

    if (image_data) {
      image_ldr.data.assign(image_data, image_data + image_ldr.size.x * image_ldr.size.y * image_ldr.channels);
      stbi_image_free(image_data);
    }

    return image_ldr;
  }

  image_hdr load_exr(const char* path) {
    image_hdr image_ldr{};

    // 1. Read EXR version.
    EXRVersion exr_version;

    int ret = ParseEXRVersionFromFile(&exr_version, path);
    if (ret != 0) {
      fprintf(stderr, "Invalid EXR file: %s\n", path);
      return {};
    }

    if (exr_version.multipart) {
      // must be multipart flag is false.
      return {};
    }

    // 2. Read EXR header
    EXRHeader exr_header;
    InitEXRHeader(&exr_header);

    const char* err = NULL; // or `nullptr` in C++11 or later.
    ret = ParseEXRHeaderFromFile(&exr_header, &exr_version, path, &err);
    if (ret != 0) {
      fprintf(stderr, "Parse EXR err: %s\n", err);
      FreeEXRErrorMessage(err); // free's buffer for an error message
      return {};
    }

    // Read HALF channel as FLOAT.
    for (int i = 0; i < exr_header.num_channels; i++) {
      if (exr_header.pixel_types[i] == TINYEXR_PIXELTYPE_HALF) {
        exr_header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
      }
    }

    image_ldr.channels = exr_header.num_channels;
    image_ldr.size.x = exr_header.data_window.max_x + 1 - exr_header.data_window.min_x;
    image_ldr.size.y = exr_header.data_window.max_y + 1 - exr_header.data_window.min_y;
    image_ldr.data.resize(image_ldr.size.x * image_ldr.size.y * image_ldr.channels);

    EXRImage exr_image;
    InitEXRImage(&exr_image);

    ret = LoadEXRImageFromFile(&exr_image, &exr_header, path, &err);
    if (ret != 0) {
      fprintf(stderr, "Load EXR err: %s\n", err);
      FreeEXRHeader(&exr_header);
      FreeEXRErrorMessage(err); // free's buffer for an error message
      return {};
    }

    // 3. Access image data
    // `exr_image.images` will be filled when EXR is scanline format.
    // `exr_image.tiled` will be filled when EXR is tiled format.

    // Read exr_image.images into new image and reorder ABGR into RGBA
    float** image_ptr = (float**)exr_image.images;

    for (int pixel = 0; pixel < image_ldr.size.x * image_ldr.size.y; pixel++) {
      for (int c = 0; c < image_ldr.channels; c++) {
        image_ldr.data[image_ldr.channels * pixel + c] = image_ptr[image_ldr.channels - 1 - c][pixel];
      }
    }

    // 4. Free image data
    FreeEXRImage(&exr_image);
    FreeEXRHeader(&exr_header);

    return image_ldr;
  }

  Texture<float> load_exr_tex(const char* path) {
    Texture<float> tex{};

    // 1. Read EXR version.
    EXRVersion exr_version;

    int ret = ParseEXRVersionFromFile(&exr_version, path);
    if (ret != 0) {
      fprintf(stderr, "Invalid EXR file: %s\n", path);
      return {};
    }

    if (exr_version.multipart) {
      // must be multipart flag is false.
      return {};
    }

    // 2. Read EXR header
    EXRHeader exr_header;
    InitEXRHeader(&exr_header);

    const char* err = NULL; // or `nullptr` in C++11 or later.
    ret = ParseEXRHeaderFromFile(&exr_header, &exr_version, path, &err);
    if (ret != 0) {
      fprintf(stderr, "Parse EXR err: %s\n", err);
      FreeEXRErrorMessage(err); // free's buffer for an error message
      return {};
    }

    // Read HALF channel as FLOAT.
    for (int i = 0; i < exr_header.num_channels; i++) {
      if (exr_header.pixel_types[i] == TINYEXR_PIXELTYPE_HALF) {
        exr_header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
      }
    }

    tex.channels = exr_header.num_channels;
    tex.dimensions.x = exr_header.data_window.max_x + 1 - exr_header.data_window.min_x;
    tex.dimensions.y = exr_header.data_window.max_y + 1 - exr_header.data_window.min_y;
    tex.dimensions.z = 1;
    tex.dimensions.w = 1;
    tex.gl_internal = gl_internal(tex.channels, 32, true);
    tex.gl_format = gl_format(tex.channels);
    tex.gl_type = gl::GLenum::GL_FLOAT;
    tex.data.resize(tex.dimensions.x * tex.dimensions.y * tex.dimensions.z * tex.dimensions.w * tex.channels);

    EXRImage exr_image;
    InitEXRImage(&exr_image);

    ret = LoadEXRImageFromFile(&exr_image, &exr_header, path, &err);
    if (ret != 0) {
      fprintf(stderr, "Load EXR err: %s\n", err);
      FreeEXRHeader(&exr_header);
      FreeEXRErrorMessage(err); // free's buffer for an error message
      return {};
    }

    // 3. Access image data
    // `exr_image.images` will be filled when EXR is scanline format.
    // `exr_image.tiled` will be filled when EXR is tiled format.

    // Read exr_image.images into new image and reorder ABGR into RGBA
    float** image_ptr = (float**)exr_image.images;

    int r = -1;
    int g = -1;
    int b = -1;
    int a = -1;

    for (int pixel = 0; pixel < tex.dimensions.x * tex.dimensions.y * tex.dimensions.z * tex.dimensions.w; pixel++) {
      for (int c = 0; c < tex.channels; c++) {
        tex.data[tex.channels * pixel + c] = image_ptr[tex.channels - 1 - c][pixel];
      }
    }

    // 4. Free image data
    FreeEXRImage(&exr_image);
    FreeEXRHeader(&exr_header);

    return tex;
  }

  bool save_jpg(const char* path, const uint8_t* data_ptr, glm::ivec2 size, int channels, int quality) {
    if (data_ptr)
      return stbi_write_jpg((std::string(path) + ".jpg").c_str(), size.x, size.y, channels, data_ptr, quality);

    return true;
  }

  bool save_png(const char* path, const uint8_t* data_ptr, glm::ivec2 size, int channels) {
    if (data_ptr)
      return stbi_write_png((std::string(path) + ".png").c_str(), size.x, size.y, channels, data_ptr, size.x * channels * sizeof(uint8_t));

    return true;
  }

  bool save_hdr(const char* path, const float* data_ptr, glm::ivec2 size, int channels) {
    if (data_ptr)
      return stbi_write_hdr((std::string(path) + ".hdr").c_str(), size.x, size.y, channels, data_ptr);

    return true;
  }

  bool save_exr_bgr(const char* path, const float* data_ptr, glm::ivec2 size, int channels) {
    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage image_ldr;
    InitEXRImage(&image_ldr);

    image_ldr.num_channels = 3;

    std::vector<float> images[3];
    images[0].resize(size.x * size.y);
    images[1].resize(size.x * size.y);
    images[2].resize(size.x * size.y);

    // Split RGBRGBRGB... into R, G and B layer
    for (int i = 0; i < size.x * size.y; i++) {
      images[0][i] = data_ptr[3 * i + 0];
      images[1][i] = data_ptr[3 * i + 1];
      images[2][i] = data_ptr[3 * i + 2];
    }

    float* image_ptr[3];
    image_ptr[0] = &(images[2].at(0)); // B
    image_ptr[1] = &(images[1].at(0)); // G
    image_ptr[2] = &(images[0].at(0)); // R

    image_ldr.images = (unsigned char**)image_ptr;
    image_ldr.width = size.x;
    image_ldr.height = size.y;

    header.num_channels = 3;
    header.channels = (EXRChannelInfo*)malloc(sizeof(EXRChannelInfo) * header.num_channels);
    // Must be (A)BGR order, since most of EXR viewers expect this channel order.
    strncpy(header.channels[0].name, "B", 255); header.channels[0].name[strlen("B")] = '\0';
    strncpy(header.channels[1].name, "G", 255); header.channels[1].name[strlen("G")] = '\0';
    strncpy(header.channels[2].name, "R", 255); header.channels[2].name[strlen("R")] = '\0';

    header.pixel_types = (int*)malloc(sizeof(int) * header.num_channels);
    header.requested_pixel_types = (int*)malloc(sizeof(int) * header.num_channels);
    for (int i = 0; i < header.num_channels; i++) {
      header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of input image
      header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF; // pixel type of output image to be stored in .EXR
    }

    const char* err = NULL; // or nullptr in C++11 or later.
    int ret = SaveEXRImageToFile(&image_ldr, &header, path, &err);
    if (ret != TINYEXR_SUCCESS) {
      fprintf(stderr, "Save EXR err: %s\n", err);
      FreeEXRErrorMessage(err); // free's buffer for an error message
      return ret;
    }
    printf("Saved exr file. [ %s ] \n", path);

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);
  }

  bool save_exr(const char* path, const float* data_ptr, glm::ivec2 size, int channels, const std::string order) {
    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage img;
    InitEXRImage(&img);

    if ((order.length() > 4) || order.length() < channels) {
      fprintf(stderr, "Save EXR err: Invalid channel order\n");
      return {};
    }

    char chan0 = (channels >= 1) ? char(order.at(0)) : '\0';
    char chan1 = (channels >= 2) ? char(order.at(1)) : '\0';
    char chan2 = (channels >= 3) ? char(order.at(2)) : '\0';
    char chan3 = (channels >= 4) ? char(order.at(3)) : '\0';

    img.width = size.x;
    img.height = size.y;
    img.num_channels = channels;

    std::vector<float> images[4];
    if (channels >= 1)
      images[0].resize(img.width * img.height);
    if (channels >= 2)
      images[1].resize(img.width * img.height);
    if (channels >= 3)
      images[2].resize(img.width * img.height);
    if (channels >= 4)
      images[3].resize(img.width * img.height);

    for (int pixel = 0; pixel < img.width * img.height; pixel++) {
      for (int c = 0; c < img.num_channels; c++) {
        images[c][pixel] = data_ptr[channels * pixel + c];
      }
    }

    float* image_ptr[4];
    for (int c = 0; c < img.num_channels; c++) {
      image_ptr[c] = &(images[img.num_channels - 1 - c].at(0));
    }

    img.images = (unsigned char**)image_ptr;
    header.num_channels = img.num_channels;
    header.channels = (EXRChannelInfo*)malloc(sizeof(EXRChannelInfo) * header.num_channels);
    // Must be BGR(A) order, since most of EXR viewers expect this channel order.
    if (header.num_channels >= 1) {
      strncpy(header.channels[0].name, &chan0, 255); header.channels[0].name[strlen(&chan0)] = '\0';
    }
    if (header.num_channels >= 2) {
      strncpy(header.channels[1].name, &chan1, 255); header.channels[1].name[strlen(&chan1)] = '\0';
    }
    if (header.num_channels >= 3) {
      strncpy(header.channels[2].name, &chan2, 255); header.channels[2].name[strlen(&chan2)] = '\0';
    }
    if (header.num_channels >= 4) {
      strncpy(header.channels[3].name, &chan3, 255); header.channels[3].name[strlen(&chan3)] = '\0';
    }

    header.pixel_types = (int*)malloc(sizeof(int) * header.num_channels);
    header.requested_pixel_types = (int*)malloc(sizeof(int) * header.num_channels);
    for (int i = 0; i < header.num_channels; i++) {
      header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of input image
      header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of output image to be stored in .EXR
    }
    //header.compression_type = TINYEXR_COMPRESSIONTYPE_NONE;

    std::string filepath = str_canonical(path);

    const char* err;
    int ret = SaveEXRImageToFile(&img, &header, filepath.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
      fprintf(stderr, "Save EXR err: %s\n", err);
      return false;
    }

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);

    return true;
  }

  bool save_exr(const char* path, const float* data_ptr, glm::ivec4 size, int channels, const std::string order) {
    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage img;
    InitEXRImage(&img);

    if ((order.length() > 4) || order.length() < channels) {
      fprintf(stderr, "Save EXR err: Invalid channel order\n");
      return {};
    }

    char chan0 = (channels >= 1) ? char(order.at(0)) : '\0';
    char chan1 = (channels >= 2) ? char(order.at(1)) : '\0';
    char chan2 = (channels >= 3) ? char(order.at(2)) : '\0';
    char chan3 = (channels >= 4) ? char(order.at(3)) : '\0';

    img.width = size.x;
    img.height = size.y;
    img.num_channels = channels;

    std::vector<float> images[4];
    if (channels >= 1)
      images[0].resize(img.width * img.height);
    if (channels >= 2)
      images[1].resize(img.width * img.height);
    if (channels >= 3)
      images[2].resize(img.width * img.height);
    if (channels >= 4)
      images[3].resize(img.width * img.height);

    header.num_channels = img.num_channels;
    header.channels = (EXRChannelInfo*)malloc(sizeof(EXRChannelInfo) * header.num_channels);
    // Must be BGR(A) order, since most of EXR viewers expect this channel order.
    if (header.num_channels >= 1) {
      strncpy(header.channels[0].name, &chan0, 255); header.channels[0].name[strlen(&chan0)] = '\0';
    }
    if (header.num_channels >= 2) {
      strncpy(header.channels[1].name, &chan1, 255); header.channels[1].name[strlen(&chan1)] = '\0';
    }
    if (header.num_channels >= 3) {
      strncpy(header.channels[2].name, &chan2, 255); header.channels[2].name[strlen(&chan2)] = '\0';
    }
    if (header.num_channels >= 4) {
      strncpy(header.channels[3].name, &chan3, 255); header.channels[3].name[strlen(&chan3)] = '\0';
    }

    header.pixel_types = (int*)malloc(sizeof(int) * header.num_channels);
    header.requested_pixel_types = (int*)malloc(sizeof(int) * header.num_channels);
    for (int i = 0; i < header.num_channels; i++) {
      header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of input image
      header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of output image to be stored in .EXR
    }
    //header.compression_type = TINYEXR_COMPRESSIONTYPE_NONE;

    uint64_t data_offset = 0;
    for (uint64_t z = 0; z < size.z; z++) {
      for (uint64_t t = 0; t < size.w; t++) {

        for (int pixel = 0; pixel < img.width * img.height; pixel++) {
          for (int c = 0; c < img.num_channels; c++) {
            images[c][pixel] = data_ptr[data_offset + channels * pixel + c];
          }
        }

        float* image_ptr[4];
        for (int c = 0; c < img.num_channels; c++) {
          image_ptr[c] = &(images[img.num_channels - 1 - c].at(0));
        }

        img.images = (unsigned char**)image_ptr;

        std::string filepath = str_canonical(path, z, t) + ".exr";

        const char* err;
        int ret = SaveEXRImageToFile(&img, &header, filepath.c_str(), &err);
        if (ret != TINYEXR_SUCCESS) {
          fprintf(stderr, "Save EXR err: %s\n", err);
          return false;
        }

        data_offset += img.width * img.height * img.num_channels;
      }
    }

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);

    return true;
  }


  bool save_jpg(const char* path, const image_ldr& img, int quality) { return save_jpg(path, img.data.data(), img.size, img.channels, quality); }
  bool save_png(const char* path, const image_ldr& img) { return save_png(path, img.data.data(), img.size, img.channels); }
  bool save_hdr(const char* path, const image_hdr& img) { return save_hdr(path, img.data.data(), img.size, img.channels); }
  bool save_exr_bgr(const char* path, const image_hdr& img) { return save_exr_bgr(path, img.data.data(), img.size, img.channels); }
  bool save_exr(const char* path, const image_hdr& img, const std::string order) { return save_exr(path, img.data.data(), img.size, img.channels, order); }
  bool save_exr(const char* path, const Texture<float>& img, const std::string order) { return save_exr(path, img.data.data(), img.dimensions, img.channels); }
}