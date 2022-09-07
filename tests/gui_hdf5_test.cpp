#include <catch2/catch_all.hpp>
#include <texpress/api.hpp>
#include <texpress/utility/normalize.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <iostream>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <thread>
#include <future>
#include <fp16.h>
#define IMGUI_COLOR_HDFGROUP ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(148/255.f, 180/255.f, 159/255.f, 255/255.f))
#define IMGUI_COLOR_HDFDATASET ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(206/255.f, 229/255.f, 208/255.f, 255/255.f))
#define IMGUI_COLOR_HDFOTHER ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(252/255.f, 248/255.f, 232/255.f, 255/255.f))
#define IMGUI_UNCOLOR ImGui::PopStyleColor()

using namespace boost::accumulators;

double average_error(const texpress::Texture& tex_a, texpress::Texture& tex_b, texpress::Encoder* enc) {
  glm::dvec4 max_err(0.0);
  glm::dvec4 min_err(INFINITY);
  glm::dvec4 error(0.0);
  uint64_t off_a = 0;
  uint64_t off_b = 0;
  uint64_t n = 0;

  texpress::Texture* slice{};

  if (tex_b.compressed()) {
    slice = new texpress::Texture();
  }
  else {
    slice = &tex_b;
  }

  for (uint64_t d = 0; d < tex_a.dimensions.z * tex_a.dimensions.w; d++) {
    if (tex_b.compressed()) {
      texpress::EncoderData input{};
      texpress::Encoder::populate_EncoderData(input, tex_b);

      texpress::EncoderData output{};
      output.data_bytes = texpress::Encoder::decoded_size(input) / (tex_a.dimensions.z * tex_a.dimensions.w);
      slice->data.resize(output.data_bytes);
      output.data_ptr = slice->data.data();

      enc->decompress(input, output, d);
      texpress::Encoder::populate_Texture(*slice, output);
    }

    for (uint64_t p = 0; p < tex_a.dimensions.x * tex_a.dimensions.y; p++) {
      glm::vec4 vec_a(0.0f, 0.0f, 0.0f, 1.0f);
      glm::vec4 vec_b(vec_a);

      for (int c = 0; c < tex_a.channels; c++) {
        vec_a[c] = tex_a.data[off_a + p * tex_a.channels + c];
      }

      for (int c = 0; c < slice->channels; c++) {
        vec_b[c] = slice->data[p * slice->channels + c];
      }

      // Avg Error
      auto diff = glm::abs(vec_a - vec_b);
      for (int i = 0; i < 4; i++) {
        max_err[i] = (diff[i] > max_err[i]) ? diff[i] : max_err[i];
        min_err[i] = (diff[i] < min_err[i]) ? diff[i] : min_err[i];
      }

      error += diff;
      n++;
    }

    off_a += (uint64_t)tex_a.bytes() / (tex_a.dimensions.z * tex_a.dimensions.w * sizeof(float));
  }

  error /= (double)n;

  spdlog::info("Source vs Compressed data");
  spdlog::info("Average: ({0:.4f}, {1:.4f}, {2:.4f}, {3:.4f})", error.x, error.y, error.z, error.w);
  spdlog::info("Maximum: ({0:.4f}, {1:.4f}, {2:.4f}, {3:.4f})", max_err.x, max_err.y, max_err.z, max_err.w);
  spdlog::info("Minimum: ({0:.4f}, {1:.4f}, {2:.4f}, {3:.4f})", min_err.x, min_err.y, min_err.z, min_err.w);

  if (tex_b.compressed()) {
    delete slice;
  }
  
  return glm::length(error)/4.0;
}

template <typename T, int length>
void angular_error(const texpress::Texture& source, const texpress::Texture& decoded, texpress::Texture& error_out) {
  if (source.dimensions != decoded.dimensions) { return; }

  // Populate error
  error_out.data.clear();
  error_out.data.resize(source.dimensions.x * source.dimensions.y * source.dimensions.z * source.dimensions.w * sizeof(float));
  error_out.dimensions = source.dimensions;
  error_out.channels = 1;
  error_out.gl_format = gl::GLenum::GL_RED;
  error_out.gl_internal = gl::GLenum::GL_R32F;
  error_out.gl_type = gl::GLenum::GL_FLOAT;
  
  const glm::vec<length, T>* src_ptr = (const glm::vec<length, T>*) source.data.data();
  const glm::vec<3, T>* dec_ptr = (const glm::vec<3, T>*) decoded.data.data();
  float* err_ptr = (float*) error_out.data.data();

  //accumulator_set<double, features<tag::mean, tag::median> > acc;

  for (auto d = 0; d < source.dimensions.z * source.dimensions.w; d++) {
    for (auto y = 0; y < source.dimensions.y; y++) {
      for (auto x = 0; x < source.dimensions.x; x++) {
        uint64_t offset = d * source.dimensions.y * source.dimensions.x + y * source.dimensions.x + x;

        const auto& v_src = glm::vec3(src_ptr[offset]);
        const auto& v_dec  = glm::vec3(dec_ptr[offset]);
        err_ptr[offset] = glm::acos(glm::dot(glm::normalize(v_src), glm::normalize(v_dec)));
        //acc(err_ptr[offset]);
      }
    }
  }

  //spdlog::info("Average angular error: {0}", mean(acc));
}

template <typename T, int length>
void distance_error(const texpress::Texture& source, const texpress::Texture& decoded, texpress::Texture& error_out) {
  if (source.dimensions != decoded.dimensions) { return; }

  // Populate error
  error_out.data.clear();
  error_out.data.resize(source.dimensions.x * source.dimensions.y * source.dimensions.z * source.dimensions.w * sizeof(float));
  error_out.dimensions = source.dimensions;
  error_out.channels = 1;
  error_out.gl_format = gl::GLenum::GL_RED;
  error_out.gl_internal = gl::GLenum::GL_R32F;
  error_out.gl_type = gl::GLenum::GL_FLOAT;

  const glm::vec<length, T>* src_ptr = (const glm::vec<length, T>*) source.data.data();
  const glm::vec<3, T>* dec_ptr = (const glm::vec<3, T>*) decoded.data.data();
  float* err_ptr = (float*)error_out.data.data();

  double avg = 0.0;

  for (auto d = 0; d < source.dimensions.z * source.dimensions.w; d++) {
    for (auto y = 0; y < source.dimensions.y; y++) {
      for (auto x = 0; x < source.dimensions.x; x++) {
        uint64_t offset = d * source.dimensions.y * source.dimensions.x + y * source.dimensions.x + x;

        const auto& v_src = glm::vec3(src_ptr[offset]);
        const auto& v_dec = glm::vec3(dec_ptr[offset]);
        err_ptr[offset] = glm::length(v_dec - v_src);
        avg += err_ptr[offset];
      }
    }
  }

  avg /= source.dimensions.w * source.dimensions.z * source.dimensions.y * source.dimensions.x;
  spdlog::info("Average distance error: {0}", avg);
}

/*
double average_error(texpress::Texture<float> tex_a, texpress::Texture<uint16_t> tex_b, texpress::Encoder* enc) {
  glm::dvec4 max_err(0.0);
  glm::dvec4 min_err(INFINITY);
  glm::dvec4 error(0.0);
  uint64_t off_a = 0;
  uint64_t off_b = 0;
  uint64_t n = 0;

  for (uint64_t d = 0; d < tex_a.dimensions.z * tex_a.dimensions.w; d++) {
    for (uint64_t p = 0; p < tex_a.dimensions.x * tex_a.dimensions.y; p++) {
      glm::vec4 vec_a(0.0f, 0.0f, 0.0f, 1.0f);
      glm::vec4 vec_b(vec_a);

      for (int c = 0; c < tex_a.channels; c++) {
        vec_a[c] = tex_a.data[off_a + p * tex_a.channels + c];
      }

      for (int c = 0; c < tex_b.channels; c++) {
        vec_b[c] = fp16_ieee_to_fp32_value(tex_b.data[off_b + p * tex_b.channels + c]);
      }

      // Avg Error
      auto diff = glm::abs(vec_a - vec_b);
      for (int i = 0; i < 4; i++) {
        max_err[i] = (diff[i] > max_err[i]) ? diff[i] : max_err[i];
        min_err[i] = (diff[i] < min_err[i]) ? diff[i] : min_err[i];
      }

      error += diff;
      n++;
    }

    off_a += (uint64_t)tex_a.bytes() / (tex_a.dimensions.z * tex_a.dimensions.w * sizeof(float));
    off_b += (uint64_t)tex_b.bytes() / (tex_b.dimensions.z * tex_b.dimensions.w * sizeof(uint16_t));
  }

  error /= (double)n;

  spdlog::info("Source vs Compressed data");
  spdlog::info("Average: ({0:.4f}, {1:.4f}, {2:.4f}, {3:.4f})", error.x, error.y, error.z, error.w);
  spdlog::info("Maximum: ({0:.4f}, {1:.4f}, {2:.4f}, {3:.4f})", max_err.x, max_err.y, max_err.z, max_err.w);
  spdlog::info("Minimum: ({0:.4f}, {1:.4f}, {2:.4f}, {3:.4f})", min_err.x, min_err.y, min_err.z, min_err.w);

  return glm::length(error) / 4.0;
}


double average_error(texpress::Texture<float> tex_a, texpress::Texture<float> tex_b, texpress::Encoder* enc) {
  glm::dvec4 max_err(0.0);
  glm::dvec4 min_err(INFINITY);
  glm::dvec4 error(0.0);
  uint64_t off_a = 0;
  uint64_t off_b = 0;
  uint64_t n = 0;

  for (uint64_t d = 0; d < tex_a.dimensions.z * tex_a.dimensions.w; d++) {
    for (uint64_t p = 0; p < tex_a.dimensions.x * tex_a.dimensions.y; p++) {
      glm::vec4 vec_a(0.0f, 0.0f, 0.0f, 1.0f);
      glm::vec4 vec_b(vec_a);

      for (int c = 0; c < tex_a.channels; c++) {
        vec_a[c] = tex_a.data[off_a + p * tex_a.channels + c];
      }

      for (int c = 0; c < tex_b.channels; c++) {
        vec_b[c] = tex_b.data[off_b + p * tex_b.channels + c];
      }

      // Avg Error
      auto diff = glm::abs(vec_a - vec_b);
      for (int i = 0; i < 4; i++) {
        max_err[i] = (diff[i] > max_err[i]) ? diff[i] : max_err[i];
        min_err[i] = (diff[i] < min_err[i]) ? diff[i] : min_err[i];
      }

      error += diff;
      n++;
    }

    off_a += (uint64_t)tex_a.bytes() / (tex_a.dimensions.z * tex_a.dimensions.w * sizeof(float));
    off_b += (uint64_t)tex_b.bytes() / (tex_b.dimensions.z * tex_b.dimensions.w * sizeof(float));
  }

  error /= (double)n;

  spdlog::info("Source vs Compressed data");
  spdlog::info("Average: ({0:.4f}, {1:.4f}, {2:.4f}, {3:.4f})", error.x, error.y, error.z, error.w);
  spdlog::info("Maximum: ({0:.4f}, {1:.4f}, {2:.4f}, {3:.4f})", max_err.x, max_err.y, max_err.z, max_err.w);
  spdlog::info("Minimum: ({0:.4f}, {1:.4f}, {2:.4f}, {3:.4f})", min_err.x, min_err.y, min_err.z, min_err.w);

  return glm::length(error) / 4.0;
}
*/

void update(double dt) {
  // noop
}

struct update_pass : texpress::render_pass
{
  update_pass(texpress::Dispatcher* adispatcher, texpress::Encoder* anencoder)
    : dispatcher(adispatcher)
    , encoder(anencoder)
    , MS_PER_UPDATE(16.6)           // Target: 60 FPS
    , clock(texpress::Wallclock())
    , t_curr(0.0)
    , t_prev(0.0)
    , lag(0.0)
    , buf_path("")
    , save_path ("")
    , load_path("")
    , buf_x("/u")
    , buf_y("/v")
    , buf_z("/w")
    , stride_x(1)
    , stride_y(1)
    , stride_z(1)
    , offset_x(0)
    , offset_y(0)
    , offset_z(0)
    , gl_tex_in(globjects::Texture::createDefault())
    , gl_tex_out(globjects::Texture::createDefault())
    , hdf5_file(nullptr)
    , tex_source()
    , tex_normalized()
    , tex_encoded()
    , tex_decoded()
    , tex_error()
    , tex_in(&tex_source)
    , tex_out(&tex_source)
    , depth_src(0)
    , depth_enc(0)
    , sync_sliders(false)
    , peaks()
  {
    on_prepare = [&] ( )
    {
      t_curr = clock.timeF64(texpress::WallclockType::WALLCLK_MS);
      t_prev = clock.timeF64(texpress::WallclockType::WALLCLK_MS);

      //strcpy(buf_path, std::filesystem::current_path().string().c_str());
      strcpy(buf_path, (std::filesystem::current_path().string() + "\\..\\files\\ctbl3d.nc").c_str());
    };
    on_update = [&] ( )
    {
      t_prev = t_curr;
      t_curr = clock.timeF64(texpress::WallclockType::WALLCLK_MS);

      double t_delta = t_curr - t_prev;
      lag += t_delta;

      while (lag >= MS_PER_UPDATE) {
        update(MS_PER_UPDATE/1000.0);
        lag -= MS_PER_UPDATE;
      }

      // Gui
      // ===
      bool* p_open = NULL;
      ImGuiWindowFlags window_flags = 0;

      // Menu
      ImGuiIO& io = ImGui::GetIO();
      ImVec2 size(io.DisplaySize.x, io.DisplaySize.y);
      ImVec2 corner(0, 0);
      window_flags |= ImGuiWindowFlags_NoTitleBar;
      ImGui::SetNextWindowSize(size);
      ImGui::SetNextWindowPos(corner);
      ImGui::Begin("Menu", p_open, window_flags);
      ImGui::Text("Texpress Menu");

      // Left menu side
      {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        //ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 260), false, window_flags);
        ImGui::BeginChild("ChildL", { ImGui::GetContentRegionAvail().x * 0.5f, ImGui::GetContentRegionAvail().y * 0.5f }, false, window_flags);
        if (ImGui::Button("Select HDF5")) {
          try {
            hdf5_file = new HighFive::File(buf_path, HighFive::File::ReadOnly);
            hdf5_structure.parse(buf_path);
          }
          catch (...)
          {
            spdlog::error("Could not load file " + std::string(buf_path));
          }
        }
        ImGui::SameLine();
        ImGui::InputText("##Filepath", buf_path, 128);

        struct Funcs {
          static int cb(ImGuiInputTextCallbackData* data) {
            if (data->EventKey) {
              spdlog::info("test");
              bool* changed = reinterpret_cast<bool*>(data->UserData);
              *changed = true;
            }

            return 0;
          }
        };

        ImGui::Text("X Dataset");
        ImGui::SameLine();
        ImGui::BeginGroup();
        if (ImGui::InputText("##X Dataset", buf_x, 128))
          configuration_changed = true;
        ImGui::Text("Stride");
        ImGui::SameLine();
        if (ImGui::InputInt("##X Stride", &stride_x, 1, 100))
          configuration_changed = true;
        ImGui::Text("Offset");
        ImGui::SameLine();
        if (ImGui::InputInt("##X Offset", &offset_x, 1, 100))
          configuration_changed = true;
        ImGui::EndGroup();

        ImGui::Text("Y Dataset");
        ImGui::SameLine();
        ImGui::BeginGroup();
        if (ImGui::InputText("##Y Dataset", buf_y, 128))
          configuration_changed = true;
        ImGui::Text("Stride");
        ImGui::SameLine();
        if (ImGui::InputInt("##Y Stride", &stride_y))
          configuration_changed = true;
        ImGui::Text("Offset");
        ImGui::SameLine();
        if (ImGui::InputInt("##Y Offset", &offset_y, 1, 100))
          configuration_changed = true;
        ImGui::EndGroup();

        ImGui::Text("Z Dataset");
        ImGui::SameLine();
        ImGui::BeginGroup();
        if (ImGui::InputText("##Z Dataset", buf_z, 128))
          configuration_changed = true;
        ImGui::Text("Stride");
        ImGui::SameLine();
        if (ImGui::InputInt("##Z Stride", &stride_z))
          configuration_changed = true;
        ImGui::Text("Offset");
        ImGui::SameLine();
        if (ImGui::InputInt("##Z Offset", &offset_z, 1, 100))
          configuration_changed = true;

        ImGui::EndGroup();

        ImGui::Checkbox("Callback", &configuration_changed);

        if (ImGui::Button("Upload HDF5")) {
          texpress::hdf5 file(buf_path);
          file.read_datasets<float>({ buf_x, buf_y, buf_z }
                                    , { uint64_t(offset_x), uint64_t(offset_y), uint64_t(offset_z) }
                                    , { uint64_t(stride_x), uint64_t(stride_y), uint64_t(stride_z) }
                                    , { 2, 1, 0 }
                                    , tex_source.data);
          tex_source.channels = file.get_vec_len(buf_x);

          auto dims = file.dataset_dimensions(buf_x);
          for (int i = 0; i < tex_source.dimensions.length(); i++) {
            tex_source.dimensions[i] = (i < dims.size()) ? dims[i] : 1;
          }

          tex_source.gl_internal = texpress::gl_internal(tex_source.channels, 32, true);
          tex_source.gl_format = texpress::gl_format(tex_source.channels);
          tex_source.gl_type = gl::GLenum::GL_FLOAT;

          tex_in = &tex_source;

          configuration_changed = false;
        }

        if (ImGui::Button("Normalize Source")) {
          tex_normalized.channels = tex_source.channels;
          tex_normalized.dimensions = tex_source.dimensions;
          tex_normalized.gl_format = tex_source.gl_format;
          tex_normalized.gl_internal = tex_source.gl_internal;
          tex_normalized.gl_type = gl::GLenum::GL_FLOAT;
          tex_normalized.data.resize(tex_source.bytes());
          peaks = texpress::find_peaks_per_component<float>(tex_source);
          float* src_ptr = (float*)tex_source.data.data();
          float* norm_ptr = (float*)tex_normalized.data.data();
          uint64_t id = 0;
          for (uint64_t d = 0; d < tex_normalized.dimensions.z * tex_normalized.dimensions.w; d++) {
            for (uint64_t p = 0; p < tex_normalized.dimensions.x * tex_normalized.dimensions.y; p++) {
              for (int c = 0; c < tex_normalized.channels; c++) {
                norm_ptr[id] = texpress::normalize_val_per_component(src_ptr[id], tex_source.channels, peaks, d, c);
                id++;
              }
            }
          }

          tex_out = &tex_normalized;
        }

        if (ImGui::Button("Denormalize Source") && !tex_normalized.data.empty()) {
          tex_decoded.channels = tex_normalized.channels;
          tex_decoded.dimensions = tex_normalized.dimensions;
          tex_decoded.gl_format = tex_normalized.gl_format;
          tex_decoded.gl_internal = tex_normalized.gl_internal;
          tex_decoded.gl_type = tex_normalized.gl_type;
          tex_decoded.data.resize(tex_normalized.bytes());

          float* norm_ptr = (float*)tex_normalized.data.data();
          float* dec_ptr = (float*)tex_decoded.data.data();
          uint64_t id = 0;
          for (uint64_t d = 0; d < tex_decoded.dimensions.z * tex_decoded.dimensions.w; d++) {
            for (uint64_t p = 0; p < tex_decoded.dimensions.x * tex_decoded.dimensions.y; p++) {
              for (int c = 0; c < tex_decoded.channels; c++) {
                dec_ptr[id] = texpress::denormalize_per_component(norm_ptr[id], tex_normalized.channels, peaks, d, c);

                id++;
              }
            }
          }

          tex_out = &tex_decoded;
        }

        static const std::vector<char*> compress_options = {"Fast", "Medium", "Production", "Highest"};
        static int compress_selected = 0;

        static bool compress_normalized = false;
        if (ImGui::Button("Compress BC6H") && !tex_source.data.empty()) {
          texpress::EncoderSettings settings{};
          settings.use_weights = false;

          if (peaks.empty()) {
            compress_normalized = false;
          }

          if (compress_normalized) {
            settings.encoding = nvtt::Format::Format_BC6S;
            //settings.encoding = nvtt::Format::Format_BC6U;
          }
          else {
            settings.encoding = nvtt::Format::Format_BC6S;
          }

          switch (compress_selected) {
          case 0:
            settings.quality = nvtt::Quality::Quality_Fastest;
            break;
          case 1:
            settings.quality = nvtt::Quality::Quality_Normal;
            break;
          case 2:
            settings.quality = nvtt::Quality::Quality_Production;
            break;
          case 3:
            settings.quality = nvtt::Quality::Quality_Highest;
            break;
          }

          settings.progress_ptr = nullptr;

          texpress::EncoderData input{};
          if (compress_normalized) {
            texpress::Encoder::populate_EncoderData(input, tex_normalized);
          }
          else {
            texpress::Encoder::populate_EncoderData(input, tex_source);
          }

          texpress::EncoderData output{};
          texpress::Encoder::initialize_buffer(tex_encoded.data, settings, input);
          texpress::Encoder::populate_EncoderData(output, tex_encoded);

          if (encoder->compress(settings, input, output)) {
            spdlog::info("Compressed!");
            texpress::Encoder::populate_Texture(tex_encoded, output);
          }

          tex_out = &tex_encoded;
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(128);
        ImGui::Combo("Quality", &compress_selected, compress_options.data(), compress_options.size());

        ImGui::SameLine();
        ImGui::Checkbox("Use Normalized Data", &compress_normalized);

        static bool decompress_and_denormalize = false;
        if (ImGui::Button("Decompress BC6H") && !tex_encoded.data.empty()) {
          texpress::EncoderData input{};
          texpress::Encoder::populate_EncoderData(input, tex_encoded);

          texpress::EncoderData output{};
          output.data_bytes = encoder->decoded_size(input);
          tex_decoded.data.resize(output.data_bytes);
          output.data_ptr = reinterpret_cast<uint8_t*>(tex_decoded.data.data());

          if (encoder->decompress(input, output)) {
            spdlog::info("Decompressed!");
          }

          texpress::Encoder::populate_Texture(tex_decoded, output);

          if (peaks.empty()) {
            decompress_and_denormalize = false;
          }

          if (decompress_and_denormalize) {
            float* dec_ptr = (float*)tex_decoded.data.data();
            uint64_t id = 0;
            for (uint64_t d = 0; d < tex_decoded.dimensions.z * tex_decoded.dimensions.w; d++) {
              for (uint64_t p = 0; p < tex_decoded.dimensions.x * tex_decoded.dimensions.y; p++) {
                for (int c = 0; c < tex_decoded.channels; c++) {
                  dec_ptr[id] = texpress::denormalize_per_component(dec_ptr[id], tex_decoded.channels, peaks, d, c);

                  id++;
                }
              }
            }
          }

          tex_out = &tex_decoded;

          configuration_changed = false;
        }

        ImGui::SameLine();
        ImGui::Checkbox("Denormalize on decompression", &decompress_and_denormalize);

        if (ImGui::Button("Save")) {
          ImGui::SetNextWindowSize(ImGui::GetContentRegionAvail());
          ImGui::OpenPopup("Save Popup");
        }

        if (ImGui::BeginPopupModal("Save Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
          static const std::vector<char*> save_options{ "Source KTX", "Normalized KTX (with Peaks)", "Compressed KTX (with Peaks)", "Decoded KTX", "Peaks", "Error" };

          static int save_selected = -1;
          if (ImGui::Combo("Select data", &save_selected, save_options.data(), save_options.size())) {
            if (save_path[0] == '\0') {
              strcpy(save_path, (std::string(buf_path) + ".ktx").c_str());
             }
          }
           
          ImGui::InputText("##Savepath", save_path, 128);

          static bool array2d = false;
          static bool monolithic = true;
          ImGui::Checkbox("As 2DArray", &array2d); ImGui::SameLine(); 
          ImGui::Checkbox("As Single File", &monolithic);

          if (ImGui::Button("Save as KTX")) {
            switch (save_selected) {
            case 0:
              if (!tex_source.data.empty()) {
                texpress::save_ktx(tex_source, save_path, array2d, monolithic);
              }
              break;
            case 1:
              if (!tex_normalized.data.empty()) {
                texpress::save_ktx(tex_normalized, save_path, array2d, monolithic);
              }
              if (!peaks.empty()) {
                std::string peaks_path = std::string(save_path);
                peaks_path = peaks_path.substr(0, peaks_path.find_last_of('.')) + ".peaks";
                texpress::file_save(peaks_path.c_str(), (char*)peaks.data(), peaks.size() * sizeof(float));
              }
              break;
            case 2:
              if (!tex_encoded.data.empty()) {
                texpress::save_ktx(tex_encoded, save_path, array2d, monolithic);
              }
              if (!peaks.empty()) {
                std::string peaks_path = std::string(save_path);
                peaks_path = peaks_path.substr(0, peaks_path.find_last_of('.')) + ".peaks";
                texpress::file_save(peaks_path.c_str(), (char*)peaks.data(), peaks.size() * sizeof(float));
              }
              break;
            case 3:
              if (!tex_decoded.data.empty()) {
                texpress::save_ktx(tex_decoded, save_path, array2d, monolithic);
              }
              break;
            case 4:
              if (!peaks.empty()) {
                std::string peaks_path = std::string(save_path);
                peaks_path = peaks_path.substr(0, peaks_path.find_last_of('.')) + ".peaks";
                texpress::file_save(peaks_path.c_str(), (char*)peaks.data(), peaks.size() * sizeof(float));
              }
              break;
            case 5:
              if (!tex_error.data.empty()) {
                texpress::save_ktx(tex_error, save_path, array2d, monolithic);
              }
              break;
            }
          }


          if (ImGui::Button("Close"))
            ImGui::CloseCurrentPopup();

          ImGui::EndPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Load")) {
          ImGui::SetNextWindowSize(ImGui::GetContentRegionAvail());
          ImGui::OpenPopup("Load Popup");
        }

        if (ImGui::BeginPopupModal("Load Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
          static const std::vector<char*> load_options{"Source KTX", "Normalized KTX", "Peaks", "Compressed KTX", "Decoded KTX", "Error KTX"};

          static int load_selected = -1;
          if (ImGui::Combo("Select data##load", &load_selected, load_options.data(), load_options.size())) {
            if (load_path[0] == '\0') {
              strcpy(load_path, (std::string(buf_path) + ".ktx").c_str());
            }
          }

          ImGui::InputText("##Loadpath", load_path, 128);

          if (ImGui::Button("Load##Action")) {
            switch (load_selected) {
            case 0:
              texpress::load_ktx(load_path, tex_source);

              tex_in = &tex_source;
              break;
            case 1:
              texpress::load_ktx(load_path, tex_normalized);

              tex_in = &tex_normalized;
              break;
            case 2:
              peaks.clear();
              peaks.resize(texpress::file_size(load_path));
              texpress::file_read(load_path, (char*)peaks.data(), peaks.size());
              break;
            case 3:
              texpress::load_ktx(load_path, tex_encoded);

              tex_out = &tex_encoded;
              break;
            case 4:
              texpress::load_ktx(load_path, tex_decoded);

              tex_out = &tex_decoded;
              break;
            case 5:
              texpress::load_ktx(load_path, tex_error);

              tex_out = &tex_error;
              break;
            }
          }


          if (ImGui::Button("Close"))
            ImGui::CloseCurrentPopup();

          ImGui::EndPopup();
        }

        if (ImGui::Button("Angular Error")) {
          if (!tex_source.data.empty()) {
            angular_error<float, 3> (tex_source, tex_decoded, tex_error);
            tex_out = &tex_error;
          }
        }

        if (ImGui::Button("Distance Error")) {
          if (!tex_source.data.empty()) {
            distance_error<float, 3>(tex_source, tex_decoded, tex_error);
            tex_out = &tex_error;
          }
        }

        if (ImGui::Button("Compare")) {
          if (!tex_source.data.empty()) {
            //average_error(tex_source, tex_encoded, encoder);
            average_error(tex_source, tex_decoded, encoder);
          }
        }

        // --> Quit
        if (ImGui::Button("Quit")) {
          spdlog::info("Quit!");
          dispatcher->post(texpress::Event(texpress::EventType::APP_SHUTDOWN));
        }

        ImGui::EndChild();
      }

      ImGui::SameLine();

      // Right menu side
      {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::BeginChild("ChildR", { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.5f }, false, window_flags);
        ImGui::Text("Source Field: "); ImGui::SameLine();
        ImGui::Text((std::to_string(tex_source.bytes() / (1024 * 1024)) + "MB").c_str()); ImGui::SameLine();
        if (ImGui::Button("Delete##source")) { tex_source.data.clear(); tex_source.data.shrink_to_fit(); }

        ImGui::Text("Normalized Field: "); ImGui::SameLine();
        ImGui::Text((std::to_string(tex_normalized.bytes() / (1024 * 1024)) + "MB").c_str()); ImGui::SameLine();
        if (ImGui::Button("Delete##normalized")) { tex_normalized.data.clear(); tex_normalized.data.shrink_to_fit();}

        ImGui::Text("Encoded Field: "); ImGui::SameLine();
        ImGui::Text((std::to_string(tex_encoded.bytes() / (1024 * 1024)) + "MB").c_str()); ImGui::SameLine();
        if (ImGui::Button("Delete##encoded")) { tex_encoded.data.clear(); tex_encoded.data.shrink_to_fit();}

        ImGui::Text("Decoded Field: "); ImGui::SameLine();
        ImGui::Text((std::to_string(tex_decoded.bytes() / (1024 * 1024)) + "MB").c_str()); ImGui::SameLine();
        if (ImGui::Button("Delete##decoded")) { tex_decoded.data.clear(); tex_decoded.data.shrink_to_fit();}

        ImGui::Text("Error Field: "); ImGui::SameLine();
        ImGui::Text((std::to_string(tex_error.bytes() / (1024 * 1024)) + "MB").c_str()); ImGui::SameLine();
        if (ImGui::Button("Delete##error")) { tex_error.data.clear(); tex_error.data.shrink_to_fit();}

        
        ImGui::Text("HDF5 File");
        // Visualize HDF5 structure
        if (!hdf5_structure.empty()) {
          //bool copy_to_clipboard = ImGui::Button("Copy");

          // Recursive ImGui tree iterator
          std::function<void(texpress::HDF5Node* node)> visualize;
          visualize = [&](texpress::HDF5Node* node) {
            // Root (group)
            if (!node->parent) {
              ImGui::SetNextItemOpen(true, ImGuiCond_Once);
              IMGUI_COLOR_HDFGROUP;
              if (ImGui::TreeNode("/")) {
                IMGUI_UNCOLOR;
                if (ImGui::IsItemHovered())
                  ImGui::SetTooltip("Root");
                for (auto* child : node->children)
                  visualize(child);
                ImGui::TreePop();
              }
              else { IMGUI_UNCOLOR; }
            }

            // Regular group
            else if (node->parent && node->is_group()) {
              IMGUI_COLOR_HDFGROUP;
              if (ImGui::TreeNode(node->name.c_str())) {
                IMGUI_UNCOLOR;
                if(ImGui::IsItemHovered())
                  ImGui::SetTooltip("Group");
                for (auto* child : node->children)
                  visualize(child);
                ImGui::TreePop();
              }
              else { IMGUI_UNCOLOR; }
              if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Group");
            }

            // Leafs
            else if (node->parent) {
              if (node->is_dataset()) { IMGUI_COLOR_HDFDATASET; }
              else {
                IMGUI_COLOR_HDFOTHER; }
              ImGui::Text(node->name.c_str());
              if (ImGui::IsItemHovered())
                  ImGui::SetTooltip(node->get_string_type().c_str());
              ImGui::SameLine();
              if (ImGui::Button(("Copy##" + node->get_path()).c_str())) {
                ImGui::LogToClipboard();
                ImGui::LogText(node->get_path().c_str());
                ImGui::LogFinish();
              }
              IMGUI_UNCOLOR;
            }
          };
          visualize(hdf5_structure.root);
        }
        ImGui::EndChild();
      }

      {
        ImGui::BeginChild("ChildBL", { ImGui::GetContentRegionAvail().x * 0.5f, ImGui::GetContentRegionAvail().y }, false, window_flags);
        if (ImGui::CollapsingHeader("Source Data")) {
          if (gl_tex_in) {
            ImGui::Checkbox("Sync Sliders", &sync_sliders);

            uint64_t min = 0;
            uint64_t max = (uint64_t)std::max(tex_in->dimensions.z * tex_in->dimensions.w - 1, 0);
            ImGui::SliderScalar("Depth Slice##src", ImGuiDataType_U64, &depth_src, &min, &max);

            if (tex_in->compressed()) {
              uint64_t offset = depth_src * (uint64_t)tex_in->bytes() / ((uint64_t)tex_in->dimensions.z * (uint64_t)tex_in->dimensions.w * (uint64_t)tex_in->bytes_type());
              gl_tex_out->compressedImage2D(0, tex_in->gl_internal, glm::ivec2(tex_in->dimensions), 0, tex_in->bytes() / (tex_in->dimensions.z * tex_in->dimensions.w), tex_in->data.data() + offset);
            }
            else {
              uint64_t offset = depth_src * (uint64_t)tex_in->dimensions.x * (uint64_t)tex_in->dimensions.y * (uint64_t)tex_in->channels * (uint64_t)tex_in->bytes_type();
              gl_tex_in->image2D(0, tex_in->gl_internal, tex_in->dimensions, 0, tex_in->gl_format, tex_in->gl_type, tex_in->data.data() + offset);
            }

            ImTextureID texID = ImTextureID(gl_tex_in->id());
            ImGui::Image(texID, ImVec2(400, 400), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0));
            ImGui::SameLine();
            ImGui::BeginGroup();
            ImGui::Text("Size: %i MB", (tex_in->bytes() / (1000 * 1000)));
            ImGui::Text("Dimensions: %ix%ix%ix%i", tex_in->dimensions.x, tex_in->dimensions.y, tex_in->dimensions.z, tex_in->dimensions.w);
            ImGui::Text("Channels: %i", tex_in->channels);
            ImGui::EndGroup();
          }
        }
        ImGui::EndChild();
      }

      ImGui::SameLine();
      
      {
        ImGui::BeginChild("ChildBR", { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y }, false, window_flags);
        if (ImGui::CollapsingHeader("Encoded Data")) {
          if (gl_tex_out) {
            ImGui::Checkbox("Sync Sliders", &sync_sliders);

            uint64_t min = 0;
            uint64_t max = (uint64_t)std::max(tex_out->dimensions.z * tex_out->dimensions.w - 1, 0);
            ImGui::SliderScalar("Depth Slice##enc", ImGuiDataType_U64, (sync_sliders) ? &depth_src : &depth_enc, &min, &max);

            if (sync_sliders) {
              depth_enc = depth_src;
            }

            if (tex_out->compressed()) {
              uint64_t offset = depth_enc * (uint64_t)tex_out->bytes() / ((uint64_t)tex_out->dimensions.z * (uint64_t)tex_out->dimensions.w * (uint64_t)tex_out->bytes_type());
              gl_tex_out->compressedImage2D(0, tex_out->gl_internal, glm::ivec2(tex_out->dimensions), 0, tex_out->bytes() / (tex_out->dimensions.z * tex_out->dimensions.w), tex_out->data.data() + offset);
            }
            else {
              uint64_t offset = depth_enc * (uint64_t)tex_out->dimensions.x * (uint64_t)tex_out->dimensions.y * (uint64_t)tex_out->channels * (uint64_t)tex_out->bytes_type();
              gl_tex_out->image2D(0, tex_out->gl_internal, tex_out->dimensions, 0, tex_out->gl_format, tex_out->gl_type, tex_out->data.data() + offset);
            }

            ImTextureID texID = ImTextureID(gl_tex_out->id());
            ImGui::Image(texID, ImVec2(400, 400), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0));
            ImGui::SameLine();
            ImGui::BeginGroup();
            ImGui::Text("Size: %i MB", (tex_out->bytes() / (1000 * 1000)));
            ImGui::Text("Dimensions: %ix%ix%ix%i", tex_out->dimensions.x, tex_out->dimensions.y, tex_out->dimensions.z, tex_out->dimensions.w);
            ImGui::Text("Channels: %i", tex_out->channels);
            ImGui::Text("Compression Ratio: %f.4%%", (uint16_t)tex_out->bytes() / std::max(tex_source.bytes(), (uint64_t)1));
            ImGui::EndGroup();
          }
        }

        ImGui::EndChild();
      }

      ImGui::End();

      ImGui::ShowDemoWindow();
    };
  }

  // Systems
  texpress::Dispatcher* dispatcher;
  texpress::Encoder* encoder;

  // Data buffers
  HighFive::File* hdf5_file;
  texpress::HDF5Tree hdf5_structure;
  texpress::Texture tex_source;
  texpress::Texture tex_normalized;
  texpress::Texture tex_encoded;
  texpress::Texture tex_decoded;
  texpress::Texture tex_error;
  texpress::Texture* tex_in;
  texpress::Texture* tex_out;

  std::unique_ptr<globjects::Texture> gl_tex_in;
  std::unique_ptr<globjects::Texture> gl_tex_out;

  // UpdateLogic
  double MS_PER_UPDATE;
  texpress::Wallclock clock;
  double t_curr;
  double t_prev;
  double lag;

  // Gui
  char buf_path[128];
  char save_path[128];
  char load_path[128];
  char buf_x[128];
  char buf_y[128];
  char buf_z[128];
  int stride_x;
  int stride_y;
  int stride_z;
  int offset_x;
  int offset_y;
  int offset_z;
  uint64_t depth_src;
  uint64_t depth_enc;
  bool configuration_changed;
  bool sync_sliders;

  uint64_t image_level;

  std::vector<float> peaks;

  // Threads
  std::thread t_encoder;
};

TEST_CASE("GUI test.", "[texpress::gui]")
{
  auto application = std::make_unique<texpress::application>();
  auto renderer    = application->add_system<texpress::renderer>();
  auto dispatcher  = application->add_system<texpress::Dispatcher>();
  auto encoder = application->add_system<texpress::Encoder>();
  ImGui::SetCurrentContext(application->gui());

  renderer->add_render_pass<texpress::render_pass>(texpress::make_clear_pass(application->window()));
  renderer->add_render_pass<texpress::render_pass>(texpress::make_prepare_pass(application->window()));
  renderer->add_render_pass<update_pass>(dispatcher, encoder);
  renderer->add_render_pass<texpress::render_pass>(texpress::make_gui_pass(application->window()));
  renderer->add_render_pass<texpress::render_pass>(texpress::make_swap_pass (application->window()));

  dispatcher->subscribe(texpress::EventType::APP_SHUTDOWN, std::bind(&texpress::application::listener, application.get(), std::placeholders::_1));
  dispatcher->subscribe(texpress::EventType::COMPRESS_BC6H, std::bind(&texpress::Encoder::listener, encoder, std::placeholders::_1));

  application->run();

  ImGui::DestroyContext();
}