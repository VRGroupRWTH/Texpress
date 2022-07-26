#include <texpress/compression/compressor.hpp>
#include <glbinding/gl45/enum.h>
#include <spdlog/spdlog.h>

namespace texpress {
  void Encoder::listener(const Event& e) {
    switch (e.mType) {
    case EventType::COMPRESS_BC6H:
      if (!e.mSendData) {
        spdlog::warn("No file has been uploaded!");
        return;
      }

      if (!e.mReceiveData) {
        spdlog::warn("No data destination provided!");
        return;
      }

      std::vector<uint8_t>* inData = static_cast<std::vector<uint8_t>*>(e.mSendData);
      std::vector<uint8_t>* outData = static_cast<std::vector<uint8_t>*>(e.mReceiveData);
      return;
    }
  }

  struct nvttOutputHandler : public nvtt::OutputHandler {
  public:
    nvttOutputHandler(uint8_t* buffer_out);
    virtual ~nvttOutputHandler();
    void beginImage(int size, int width, int height, int depth, int face, int miplevel);
    bool writeData(const void* data, int size);
    void endImage();
    void setTotal(uint64_t t);
    void setProgressOutput(int* ptr);

  private:
    uint8_t* buffer;
    uint64_t offset;
    uint64_t total;
    uint64_t progress;
    int percentage;
    int* progress_output;
  };


  nvttOutputHandler::nvttOutputHandler(uint8_t* buffer_out) :
      buffer(buffer_out)
    , offset(0)
    , total(0)
    , progress(0)
    , percentage(0)
    , progress_output(nullptr)
    {}

  nvttOutputHandler::~nvttOutputHandler() {
    buffer = nullptr;
    progress_output = nullptr;
  }

  void nvttOutputHandler::beginImage(int size, int width, int height, int depth, int face, int miplevel) {
    // ignore
  }

  void nvttOutputHandler::endImage() {
    // ignore
  }

  bool nvttOutputHandler::writeData(const void* data, int size) {
    if (!data)
      return false;

    //memcpy((void*)(buffer + offset), data, size);

    uint8_t* ptr = (uint8_t*) data;
    for (auto i = 0; i < size; i++) {
      buffer[offset + i] = ptr[i];
    }
    offset += size;

    // Progress
    progress += size;
    int p = int((100 * progress) / total);
    if (p != percentage) {
      percentage = p;

      if (progress_output) {
        *progress_output = percentage;
      }
      else {
        printf("\r%d%%", percentage);
        fflush(stdout);
      }
    }
    return true;
  }

  void nvttOutputHandler::setTotal(uint64_t t) {
    total = t;
  }

  void nvttOutputHandler::setProgressOutput(int* ptr) {
    progress_output = ptr;
  }


  uint64_t Encoder::estimate_size(const EncoderSettings& settings, const EncoderData& input) {
    nvtt::Context context(false);

    // Specify what compression settings to use
    nvtt::CompressionOptions compressionOptions;
    compressionOptions.setFormat(settings.encoding);
    compressionOptions.setQuality(settings.quality);
    if (settings.use_weights) {
      compressionOptions.setColorWeights(settings.red_weight, settings.green_weight, settings.blue_weight, settings.alpha_weight);
    }

    // Setup empty floating-point RGBA image to deduce needed buffersize.
    nvtt::Surface image;
    image.setImage(nvtt::InputFormat_RGBA_32F, input.dim_x, input.dim_y, 1, input.data_ptr);
    uint64_t buffer_size = context.estimateSize(image, 1, compressionOptions) * input.dim_z * input.dim_t;

    return buffer_size;
  }

  bool Encoder::compress(const EncoderSettings& settings, const EncoderData& input, EncoderData& output) {
    if (!output.data_ptr) {
      spdlog::error("Output Buffer not initialized");
      return false;
    }

    if ((gl::GLenum)input.gl_format != gl::GLenum::GL_RGBA) {
      spdlog::error("gl_format must be RGBA.");
      return false;
    }

    // Create context which enables CUDA compression for capable GPUs.
    // Incapable GPUs will fall back to CPU compression.
    nvtt::Context context(false);

    // Specify what compression settings to use
    nvtt::CompressionOptions compressionOptions;
    compressionOptions.setFormat(settings.encoding);
    compressionOptions.setQuality(settings.quality);
    if (settings.use_weights) {
      compressionOptions.setColorWeights(settings.red_weight, settings.green_weight, settings.blue_weight, settings.alpha_weight);
    }

    // Setup empty floating-point RGBA image to deduce needed buffersize.
    nvtt::Surface image;
    image.setImage(nvtt::InputFormat_RGBA_32F, input.dim_x, input.dim_y, 1, input.data_ptr);
    uint64_t buffer_size = context.estimateSize(image, 1, compressionOptions) * input.dim_z * input.dim_t;

    if (buffer_size > output.data_bytes) {
      spdlog::error("Output Buffer {0} bytes too small", buffer_size - output.data_bytes);
      return false;
    }

    // Custom output handler to show progress.
    nvttOutputHandler outputHandler(output.data_ptr);
    outputHandler.setTotal(buffer_size);
    if (settings.progress_ptr) {
      outputHandler.setProgressOutput(settings.progress_ptr);
    }

    // Official output handler which registers the custom handler.
    nvtt::OutputOptions outputOptions;
    outputOptions.setOutputHandler((nvtt::OutputHandler*)&outputHandler);

    // Prepare output
    output.dim_x = input.dim_x;
    output.dim_y = input.dim_y;
    output.dim_z = input.dim_z;
    output.dim_t = input.dim_t;
    output.channels = input.channels;
    
    switch (settings.encoding) {
    case nvtt::Format_BC6S:
      output.gl_internal = (uint32_t) gl::GLenum::GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
      break;
    case nvtt::Format_BC6U:
      output.gl_internal = (uint32_t) gl::GLenum::GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
      break;
    default:
      spdlog::error("Encoding {0} is unsupported, use BC6H.", settings.encoding);
      return false;
    }

    // Compress whole (time) volume
    uint64_t offset = 0;
    for (int t = 0; t < input.dim_t; t++) {
      for (int z = 0; z < input.dim_z; z++) {
        image.setImage(nvtt::InputFormat_RGBA_32F, input.dim_x, input.dim_y, 1, input.data_ptr + offset);

        if (!context.compress(image, 0, 0, compressionOptions, outputOptions)) {
          spdlog::error("Compression failed");
          return false;
        }

        offset += input.dim_x * input.dim_y * input.channels;
      }
    }

    return true;
  }


  Texture<uint8_t> Encoder::compress_bc6h_nvtt(const Texture<float>& input) {
    // Output structure
    auto out = Texture<uint8_t>{};
    out.data_channels = input.data_channels;
    out.grid_size = input.grid_size;
    out.gl_type = TEXPRESS_FLOAT;
    out.gl_pixelFormat = gl_pixel(out.data_channels);
    out.gl_internalFormat = TEXPRESS_BC6H_SIGNED;
    out.enc_blocksize = { 4, 4, 1 };

    // Load the source image into a floating-point RGBA image.
    nvtt::Surface image;
    image.setImage(nvtt::InputFormat_RGBA_32F, out.grid_size.x, out.grid_size.y, out.grid_size.z, input.data.data());

    // Context
    // Create the compression context; enable CUDA compression, so that
    // CUDA-capable GPUs will use GPU acceleration for compression, with a
    // fallback on other GPUs for CPU compression.
    nvtt::Context context(false);

    // Specify what compression settings to use. In our case the only default
    // to change is that we want to compress to BC7.
    nvtt::CompressionOptions compressionOptions;
    compressionOptions.setFormat(nvtt::Format_BC6S);
    compressionOptions.setQuality(nvtt::Quality_Fastest);
    //compressionOptions.setColorWeights(1.0, 1.0, 1.0);

    out.data_size = context.estimateSize(image, 1, compressionOptions);

    // Specify how to output the compressed data. Here, we say to write to a file.
    // We could also use a custom output handler here instead.
    out.data.resize(out.data_size);
    nvttOutputHandler outputHandler(out.data.data());
    outputHandler.setTotal(out.data_size);
    nvtt::OutputOptions outputOptions;

    outputOptions.setOutputHandler((nvtt::OutputHandler*)&outputHandler);
    //if (!context.compress(image, 0, 0, compressionOptions, outputOptions)) {
    //  spdlog::error("Compression failed");
    //}

    uint64_t offset = 0;
    for (int t = 0; t < input.grid_size.w; t++) {
      for (int z = 0; z < input.grid_size.z; z++) {
        nvtt::Surface img;
        img.setImage(nvtt::InputFormat_RGBA_32F, out.grid_size.x, out.grid_size.y, 1, (input.data.data() + offset));

        if (!context.compress(img, 0, 0, compressionOptions, outputOptions)) {
          spdlog::error("Compression failed");
        }
        
        offset += input.grid_size.x * input.grid_size.y * input.data_channels;
      }

      offset += input.grid_size.x * input.grid_size.y * input.grid_size.z * input.data_channels;
    }

    return out;
  }

  Texture<float> Encoder::decompress_bc6h_nvtt(const Texture<uint8_t>& input) {
    // Output structure
    auto out = Texture<float>{};
    out.grid_size = input.grid_size;
    out.gl_type = TEXPRESS_FLOAT;
    out.data_channels = input.data_channels;
    out.gl_pixelFormat = gl_pixel(out.data_channels);
    out.gl_internalFormat = gl_internal_uncomnpressed(out.data_channels, 32, true);
    out.data_size = out.grid_size.x * out.grid_size.y * out.grid_size.z * out.grid_size.w * out.data_channels;
    // Load the source image into a floating-point RGBA image.
    nvtt::Surface image;
    image.setImage3D(nvtt::Format_BC6S, input.grid_size.x, input.grid_size.y, input.grid_size.z, input.data.data());
    const float* r_ptr = (out.data_channels >= 1) ? image.channel(0) : nullptr;
    const float* g_ptr = (out.data_channels >= 2) ? image.channel(1) : nullptr;
    const float* b_ptr = (out.data_channels >= 3) ? image.channel(2) : nullptr;
    const float* a_ptr = (out.data_channels >= 4) ? image.channel(3) : nullptr;

    out.data.resize(out.data_size);
    for (auto p = 0; p < out.grid_size.x * out.grid_size.y * out.grid_size.z * out.grid_size.w; p++) {
      out.data[p * out.data_channels + 0] = r_ptr[p];
      out.data[p * out.data_channels + 1] = g_ptr[p];
      out.data[p * out.data_channels + 2] = b_ptr[p];

      if (a_ptr) {
        out.data[p * out.data_channels + 3] = a_ptr[p];
      }
    }

    return out;
  }
}