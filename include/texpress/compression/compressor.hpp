#pragma once
#include <texpress/export.hpp>
#include <texpress/defines.hpp>
#include <texpress/compression/compressor_defines.hpp>

#include <texpress/core/system.hpp>
#include <texpress/events/event.hpp>
#include <texpress/types/texture.hpp>
#include <texpress/types/image.hpp>

#include <nvtt/nvtt.h>
#include <atomic>

namespace texpress
{
  struct TEXPRESS_EXPORT EncoderData {
    uint32_t gl_internal = 0;       // OpenGL internalformat, https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
    uint32_t gl_format = 0;         // OpenGL format, https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
    uint32_t dim_x = 0;             // Extents of each dimension
    uint32_t dim_y = 0;
    uint32_t dim_z = 0;
    uint32_t dim_t = 0;
    uint8_t channels = 0;           // Number of colorchannels
    uint64_t data_bytes = 0;        // Size of buffer, should be: dim_x * dim_y * dim_z * dim_t * channels * sizeof(Type)
    uint8_t* data_ptr = nullptr;    // Data to be read from or written to
  };

  struct TEXPRESS_EXPORT EncoderSettings{
    bool use_weights = false;                                 // If true colorchannels are weighted differently to estimate compression error
    float red_weight = 1.0;                                   // Weights of each channel
    float green_weight = 1.0;
    float blue_weight = 1.0;
    float alpha_weight = 1.0;
    nvtt::Quality quality = nvtt::Quality::Quality_Fastest;   // Encoding quality, for BC6H only "Fastest" and "Normal" are available
    nvtt::Format encoding = nvtt::Format::Format_BC6S;        // Target encoding
    int* progress_ptr = nullptr;                            // Use this if you want to display compression progress else than in console
  };

  class TEXPRESS_EXPORT Encoder : public system
  {
  public:
    Encoder() = default;
    Encoder(const Encoder& that) = delete;
    Encoder(Encoder&& temp) = delete;
    ~Encoder() = default;
    Encoder& operator=(const Encoder& that) = delete;
    Encoder& operator=(Encoder&& temp) = delete;

    void listener(const Event& e);

  public:
    static uint64_t decoded_size(const EncoderData& input, bool hdr = true);
    static uint64_t encoded_size(const EncoderSettings& settings, const EncoderData& input);
    static bool populate_EncoderData(EncoderData& enc_data, Texture& tex_input);
    static bool populate_EncoderData(EncoderData& enc_data, image& img_input);
    static bool populate_Texture(Texture& tex, EncoderData& enc_data);
    static bool populate_Image(image& img, EncoderData& enc_data);

    static void initialize_buffer(std::vector<uint8_t>& buffer, const EncoderSettings& settings, const EncoderData& input) {
      buffer.resize(encoded_size(settings, input)/sizeof(uint8_t));
    }

    template <typename T>
    static void initialize_buffer(std::vector<T>& buffer, const EncoderSettings& settings, const EncoderData& input) {
      buffer.resize(encoded_size(settings, input) / sizeof(T));
    }

    template <typename T>
    static void initialize_buffer(T* buffer, const EncoderSettings& settings, const EncoderData& input) {
      buffer = new T[(encoded_size(settings, input) / sizeof(double))];
    }

    template <typename T>
    static void free_buffer(std::vector<T>& buffer) {
      buffer.clear();
    }

    template <typename T>
    static void free_buffer(T* buffer) {
      delete buffer;
    }

    bool compress(const EncoderSettings& settings, const EncoderData& input, EncoderData& output);
    bool decompress(const EncoderData& input, EncoderData& output);
    bool decompress(const EncoderData& input, EncoderData& output, uint64_t slice);
    bool decompress(const nvtt::Format encoding, const EncoderData& input, EncoderData& output);
    bool decompress(const nvtt::Format encoding, const EncoderData& input, EncoderData& output, uint64_t slice);

    //static Texture<uint8_t> compress_bc6h_nvtt(const Texture<float>& input);
    //static Texture<float> decompress_bc6h_nvtt(const Texture<uint8_t>& input);

  private:
    static bool populate_EncoderData_base(EncoderData& enc_data, uint32_t dim_x, uint32_t dim_y, uint32_t dim_z, uint32_t dim_t, uint8_t channels, uint64_t data_bytes, uint8_t* data_ptr);

  private:
    std::atomic<bool> busy = false;
  };
}