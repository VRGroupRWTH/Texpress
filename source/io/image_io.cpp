#include <texpress/io/image_io.hpp>
#include <string>
#include <texpress/utility/stringtools.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace texpress {
    image load_image(const char* path, int requested_channels) {
        image image_ldr{};
        uint8_t* image_data = stbi_load(path, &image_ldr.size.x, &image_ldr.size.y, &image_ldr.channels, requested_channels);
        if (requested_channels != 0)
            image_ldr.channels = requested_channels;

        if (image_data) {
            image_ldr.data.assign(image_data, image_data + image_ldr.size.x * image_ldr.size.y * image_ldr.channels);
            stbi_image_free(image_data);
        }

        return image_ldr;
    }

    image load_image_hdr(const char* path, int requested_channels) {
        image image_hdr{};
        image_hdr.hdr = true;
        float* image_data = stbi_loadf(path, &image_hdr.size.x, &image_hdr.size.y, &image_hdr.channels, requested_channels);
        if (requested_channels != 0)
            image_hdr.channels = requested_channels;

        if (image_data) {
            image_hdr.data.assign(image_data, image_data + image_hdr.size.x * image_hdr.size.y * image_hdr.channels * sizeof(float));
            stbi_image_free(image_data);
        }

        return image_hdr;
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

    bool save_hdr(const char* path, const uint8_t* data_ptr, glm::ivec2 size, int channels) {
        if (data_ptr)
            return stbi_write_hdr((std::string(path) + ".hdr").c_str(), size.x, size.y, channels, (float*)data_ptr);

        return true;
    }


    bool save_jpg(const char* path, const image& img, int quality) { return save_jpg(path, img.data.data(), img.size, img.channels, quality); }
    bool save_png(const char* path, const image& img) { return save_png(path, img.data.data(), img.size, img.channels); }
    bool save_hdr(const char* path, const image& img) { return save_hdr(path, img.data.data(), img.size, img.channels); }
}