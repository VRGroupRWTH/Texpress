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
    uint64_t total;
    uint64_t progress;
    int lastwritten;
    int percentage;
    int* progress_output;
  };


  nvttOutputHandler::nvttOutputHandler(uint8_t* buffer_out) :
      buffer(buffer_out)
    , total(0)
    , progress(0)
    , percentage(0)
    , progress_output(nullptr)
    , lastwritten(0)
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
    for (uint32_t i = 0; i < size; i++) {
      buffer[progress + i] = ptr[i];
    }

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

  uint64_t Encoder::encoded_size(const EncoderSettings& settings, const EncoderData& input) {
    nvtt::Context context(false);

    // Specify what compression settings to use
    nvtt::CompressionOptions compressionOptions;
    compressionOptions.setFormat(settings.encoding);
    compressionOptions.setQuality(settings.quality);
    if (settings.use_weights) {
      compressionOptions.setColorWeights(settings.red_weight, settings.green_weight, settings.blue_weight, settings.alpha_weight);
    }

    // Setup empty floating-point RGBA image to deduce needed buffersize.
    nvtt::Surface surface;
    surface.setImage(nvtt::InputFormat_RGBA_32F, input.dim_x, input.dim_y, 1, input.data_ptr);
    uint64_t buffer_size = context.estimateSize(surface, 1, compressionOptions) * input.dim_z * input.dim_t;

    return buffer_size;
  }

  uint64_t Encoder::decoded_size(const EncoderData& input, bool hdr) {
    uint64_t element_bytes = (hdr) ? sizeof(float) : sizeof(uint8_t);
    return input.dim_x * input.dim_y * input.dim_z * input.dim_t * input.channels * element_bytes;
  }

  bool Encoder::compress(const EncoderSettings& settings, const EncoderData& input, EncoderData& output) {
    // Multithread safety
    if (busy.exchange(true)) {
      return false;
    }

    if (!output.data_ptr) {
      spdlog::error("Output Buffer not initialized");
      busy.store(false);
      return false;
    }

    if ((gl::GLenum)input.gl_format != gl::GLenum::GL_RGBA) {
      spdlog::error("gl_format must be RGBA.");
      busy.store(false);
      return false;
    }

    // Create context which enables CUDA compression for capable GPUs.
    // Incapable GPUs will fall back to CPU compression.
    nvtt::Context context(true);

    // Specify what compression settings to use
    nvtt::CompressionOptions compressionOptions;
    compressionOptions.setFormat(settings.encoding);
    compressionOptions.setQuality(settings.quality);
    if (settings.use_weights) {
      compressionOptions.setColorWeights(settings.red_weight, settings.green_weight, settings.blue_weight, settings.alpha_weight);
    }

    // Setup empty floating-point RGBA image to deduce needed buffersize.
    nvtt::Surface surface;
    surface.setImage(nvtt::InputFormat_RGBA_32F, input.dim_x, input.dim_y, 1, input.data_ptr);
    uint64_t buffer_size = context.estimateSize(surface, 1, compressionOptions) * input.dim_z * input.dim_t;

    if (buffer_size > output.data_bytes) {
      spdlog::error("Output Buffer {0} bytes too small", buffer_size - output.data_bytes);
      busy.store(false);
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

      busy.store(false);
      return false;
    }

    // Compress whole (time) volume
    uint64_t offset = 0;
    int written = 0;
    for (int t = 0; t < input.dim_t; t++) {
      for (int z = 0; z < input.dim_z; z++) {
        nvtt::Surface surface2;
        surface2.setImage(nvtt::InputFormat_RGBA_32F, input.dim_x, input.dim_y, 1, input.data_ptr + offset);

        if (!context.compress(surface2, 0, 0, compressionOptions, outputOptions)) {
          spdlog::error("Compression failed");
          busy.store(false);
          return false;
        }

        offset += input.data_bytes / (input.dim_t * input.dim_z);
      }
    }

    /*
    surface.setImage(nvtt::InputFormat_RGBA_32F, input.dim_x, input.dim_y, input.dim_z, input.data_ptr);

    if (!context.compress(surface, 0, 0, compressionOptions, outputOptions)) {
      spdlog::error("Compression failed");
      return false;
    }
    */
    busy.store(false);
    return true;
  }

  bool Encoder::decompress(const EncoderData& input, EncoderData& output) {
    // Multithread safety
    if (busy.exchange(true)) {
      return false;
    }

    if (input.gl_internal == (uint32_t)gl::GLenum::GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT) {
      busy.store(false);
      return decompress(nvtt::Format::Format_BC6S, input, output);
    }

    if (input.gl_internal == (uint32_t)gl::GLenum::GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT) {
      busy.store(false);
      return decompress(nvtt::Format::Format_BC6U, input, output);
    }

    spdlog::error("Could not deduce nvtt encoding of gl_internal with value {0}.", input.gl_internal);
    busy.store(false);
    return false;
  }

  bool Encoder::decompress(const nvtt::Format encoding, const EncoderData& input, EncoderData& output) {
    // Multithread safety
    if (busy.exchange(true)) {
      return false;
    }

    if (!output.data_ptr) {
      spdlog::error("Output Buffer not initialized");
      busy.store(false);
      return false;
    }

    uint64_t buffer_size = input.dim_x * input.dim_y * input.dim_z * input.dim_t * input.channels * sizeof(float);
    if (buffer_size > output.data_bytes) {
      spdlog::error("Output Buffer {0} bytes too small", buffer_size - output.data_bytes);
      busy.store(false);
      return false;
    }

    // Setup empty floating-point RGBA image to deduce needed buffersize.
    nvtt::Surface surface;
    surface.setImage3D(encoding, input.dim_x, input.dim_y, input.dim_z, input.data_ptr);

    const float* r_ptr = (input.channels >= 1) ? surface.channel(0) : nullptr;
    const float* g_ptr = (input.channels >= 2) ? surface.channel(1) : nullptr;
    const float* b_ptr = (input.channels >= 3) ? surface.channel(2) : nullptr;
    const float* a_ptr = (input.channels >= 4) ? surface.channel(3) : nullptr;

    float* out_ptr = reinterpret_cast<float*>(output.data_ptr);
    for (auto p = 0; p < input.dim_x * input.dim_y * input.dim_z * input.dim_t; p++) {
      if (r_ptr) {
        out_ptr[p * input.channels + 0] = r_ptr[p];
      }
      if (g_ptr) {
        out_ptr[p * input.channels + 1] = g_ptr[p];
      }
      if (b_ptr) {
        out_ptr[p * input.channels + 2] = b_ptr[p];
      }
      if (a_ptr) {
        out_ptr[p * input.channels + 3] = a_ptr[p];
      }
    }

    // Prepare output
    output.dim_x = input.dim_x;
    output.dim_y = input.dim_y;
    output.dim_z = input.dim_z;
    output.dim_t = input.dim_t;
    output.channels = input.channels;
    //output.gl_format = (uint32_t)gl::GLenum::GL_RGBA;
    //output.gl_internal = (uint32_t)gl::GLenum::GL_RGBA32F;
    output.gl_format = (uint32_t) gl_internal(input.channels, 32, true);
    output.gl_format = (uint32_t) gl_format(input.channels);

    busy.store(false);
    return true;
  }

  bool Encoder::populate_EncoderData(EncoderData& enc_data, Texture<float>& tex_input) {
    enc_data.gl_format = (uint32_t) tex_input.gl_format;
    enc_data.gl_internal= (uint32_t)tex_input.gl_internal;

    return populate_EncoderData_base(enc_data,
      tex_input.dimensions.x, tex_input.dimensions.y, tex_input.dimensions.z, tex_input.dimensions.w,
      tex_input.channels, tex_input.bytes(), reinterpret_cast<uint8_t*>(tex_input.data.data())
    );
  }

  bool Encoder::populate_EncoderData(EncoderData& enc_data, Texture<uint8_t>& tex_input) {
    enc_data.gl_format = (uint32_t)tex_input.gl_format;
    enc_data.gl_internal = (uint32_t)tex_input.gl_internal;

    return populate_EncoderData_base(enc_data,
      tex_input.dimensions.x, tex_input.dimensions.y, tex_input.dimensions.z, tex_input.dimensions.w,
      tex_input.channels, tex_input.bytes(), tex_input.data.data()
    );
  }
  bool Encoder::populate_EncoderData(EncoderData& enc_data, image_ldr& img_input) {
    enc_data.gl_format = (uint32_t) gl_format(img_input.channels);
    enc_data.gl_internal = (uint32_t)gl_internal(img_input.channels, 8, false);

    return populate_EncoderData_base(enc_data,
      img_input.size.x, img_input.size.y, 1, 1,
      img_input.channels, img_input.bytes(), img_input.data.data()
    );
  }
  bool Encoder::populate_EncoderData(EncoderData& enc_data, image_hdr& img_input) {
    enc_data.gl_format = (uint32_t)gl_format(img_input.channels);
    enc_data.gl_internal = (uint32_t)gl_internal(img_input.channels, 32, true);

    return populate_EncoderData_base(enc_data,
      img_input.size.x, img_input.size.y, 1, 1,
      img_input.channels, img_input.bytes(), reinterpret_cast<uint8_t*>(img_input.data.data())
    );
  }

  bool Encoder::populate_Texture(Texture<float>& tex, EncoderData& enc_data) {
    tex.dimensions = { enc_data.dim_x, enc_data.dim_y, enc_data.dim_z, enc_data.dim_t };
    tex.channels = enc_data.channels;

    tex.gl_internal = gl::GLenum(enc_data.gl_internal);
    tex.gl_format = gl::GLenum(enc_data.gl_format);
    tex.gl_type = gl::GL_FLOAT;

    if (tex.gl_internal == gl::GLenum::GL_ZERO) {
      tex.gl_internal = gl_internal(enc_data.channels, 32, true);
      //uint8_t bits = (enc_data.data_bytes / (enc_data.dim_x * enc_data.dim_y * enc_data.dim_z * enc_data.dim_t * enc_data.channels)) * 8;
      //tex.gl_internal = gl_internal_uncomnpressed(enc_data.channels, bits, bits > 8);
    }

    return true;
  }

  bool Encoder::populate_Texture(Texture<uint8_t>& tex, EncoderData& enc_data) {
    tex.dimensions = { enc_data.dim_x, enc_data.dim_y, enc_data.dim_z, enc_data.dim_t };
    tex.channels = enc_data.channels;

    tex.gl_internal = gl::GLenum(enc_data.gl_internal);
    tex.gl_format = gl::GLenum(enc_data.gl_format);
    tex.gl_type = gl::GL_UNSIGNED_BYTE;

    if (tex.gl_internal == gl::GLenum::GL_ZERO) {
      tex.gl_internal = gl_internal(enc_data.channels, 8, false);
      //uint8_t bits = (enc_data.data_bytes / (enc_data.dim_x * enc_data.dim_y * enc_data.dim_z * enc_data.dim_t * enc_data.channels)) * 8;
      //tex.gl_internal = gl_internal_uncomnpressed(enc_data.channels, bits, bits > 8);
    }

    return true;
  }

  bool Encoder::populate_Image(image& img, EncoderData& enc_data) {
    img.size = { enc_data.dim_x, enc_data.dim_y };
    img.channels = enc_data.channels;

    return true;
  }

  bool Encoder::populate_EncoderData_base(EncoderData& enc_data, uint32_t dim_x, uint32_t dim_y, uint32_t dim_z, uint32_t dim_t, uint8_t channels, uint64_t data_bytes, uint8_t* data_ptr) {
    enc_data.dim_x = dim_x;
    enc_data.dim_y = dim_y;
    enc_data.dim_z = dim_z;
    enc_data.dim_t = dim_t;
    enc_data.channels = channels;
    enc_data.data_bytes = data_bytes;
    enc_data.data_ptr = data_ptr;

    return true;
  }

  /*
  Texture<uint8_t> Encoder::compress_bc6h_nvtt(const Texture<float>& input) {
    // Output structure
    auto out = Texture<uint8_t>{};
    out.channels = input.channels;
    out.dimensions = input.dimensions;
    out.gl_type = TEXPRESS_FLOAT;
    out.gl_format = gl_pixel(out.channels);
    out.gl_internal = TEXPRESS_BC6H_SIGNED;
    out.enc_blocksize = { 4, 4, 1 };

    // Load the source image into a floating-point RGBA image.
    nvtt::Surface image;
    image.setImage(nvtt::InputFormat_RGBA_32F, out.dimensions.x, out.dimensions.y, out.dimensions.z, input.data.data());

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
    for (int t = 0; t < input.dimensions.w; t++) {
      for (int z = 0; z < input.dimensions.z; z++) {
        nvtt::Surface img;
        img.setImage(nvtt::InputFormat_RGBA_32F, out.dimensions.x, out.dimensions.y, 1, (input.data.data() + offset));

        if (!context.compress(img, 0, 0, compressionOptions, outputOptions)) {
          spdlog::error("Compression failed");
        }
        
        offset += input.dimensions.x * input.dimensions.y * input.channels;
      }

      offset += input.dimensions.x * input.dimensions.y * input.dimensions.z * input.channels;
    }

    return out;
  }

  Texture<float> Encoder::decompress_bc6h_nvtt(const Texture<uint8_t>& input) {
    // Output structure
    auto out = Texture<float>{};
    out.dimensions = input.dimensions;
    out.gl_type = TEXPRESS_FLOAT;
    out.channels = input.channels;
    out.gl_format = gl_pixel(out.channels);
    out.gl_internal = gl_internal_uncomnpressed(out.channels, 32, true);
    out.data_size = out.dimensions.x * out.dimensions.y * out.dimensions.z * out.dimensions.w * out.channels;
    // Load the source image into a floating-point RGBA image.
    nvtt::Surface image;
    image.setImage3D(nvtt::Format_BC6S, input.dimensions.x, input.dimensions.y, input.dimensions.z, input.data.data());
    const float* r_ptr = (out.channels >= 1) ? image.channel(0) : nullptr;
    const float* g_ptr = (out.channels >= 2) ? image.channel(1) : nullptr;
    const float* b_ptr = (out.channels >= 3) ? image.channel(2) : nullptr;
    const float* a_ptr = (out.channels >= 4) ? image.channel(3) : nullptr;

    out.data.resize(out.data_size);
    for (auto p = 0; p < out.dimensions.x * out.dimensions.y * out.dimensions.z * out.dimensions.w; p++) {
      out.data[p * out.channels + 0] = r_ptr[p];
      out.data[p * out.channels + 1] = g_ptr[p];
      out.data[p * out.channels + 2] = b_ptr[p];

      if (a_ptr) {
        out.data[p * out.channels + 3] = a_ptr[p];
      }
    }

    return out;
  }
  */
}