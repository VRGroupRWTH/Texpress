#include <texpress/api.hpp>
#include <texpress/helpers/vtkhelper.hpp>
#include <texpress/utility/normalize.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <fp16/fp16.h>
#include <iostream>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <thread>
#include <future>
#include <fp16.h>
#include <chrono>
#include <glm/gtx/compatibility.hpp>
#include <texpress/utility/stringtools.hpp>
#define IMGUI_COLOR_HDFGROUP ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(148/255.f, 180/255.f, 159/255.f, 255/255.f))
#define IMGUI_COLOR_HDFDATASET ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(206/255.f, 229/255.f, 208/255.f, 255/255.f))
#define IMGUI_COLOR_HDFOTHER ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(252/255.f, 248/255.f, 232/255.f, 255/255.f))
#define IMGUI_UNCOLOR ImGui::PopStyleColor()
#define PI 3.14159265359

using namespace boost::accumulators;


void component_error(const texpress::Texture& tex_a, const texpress::Texture& tex_b, texpress::Texture& tex_error) {
    tex_error.data.clear();
    tex_error.dimensions = tex_a.dimensions;
    tex_error.channels = tex_a.channels;
    tex_error.gl_internal = texpress::gl_internal(tex_error.channels, 32, true);
    tex_error.gl_format = texpress::gl_format(tex_error.channels);
    tex_error.gl_type = gl::GLenum::GL_FLOAT;
    tex_error.data.resize(tex_a.bytes());

    glm::dvec4 avg_error(0.0);

    const float* a_ptr = (const float*)tex_a.data.data();
    const float* b_ptr = (const float*)tex_b.data.data();
    float* err_ptr = (float*)tex_error.data.data();

    uint64_t vectors_2d = (uint64_t)tex_a.dimensions.x * (uint64_t)tex_a.dimensions.y;
    uint64_t scalars_2d = vectors_2d * (uint64_t)tex_a.channels;

    uint64_t n = vectors_2d * (uint64_t)tex_a.dimensions.z * (uint64_t)tex_a.dimensions.w;

    for (uint64_t d = 0; d < tex_a.dimensions.z * tex_a.dimensions.w; d++) {
        for (uint64_t p = 0; p < tex_a.dimensions.x * tex_a.dimensions.y; p++) {
            glm::vec4 vec_a(0.0f, 0.0f, 0.0f, 1.0f);
            glm::vec4 vec_b(vec_a);

            for (uint64_t c = 0; c < tex_a.channels; c++) {
                vec_a[c] = a_ptr[d * scalars_2d + p * (uint64_t)tex_a.channels + c];
                vec_b[c] = b_ptr[d * scalars_2d + p * (uint64_t)tex_b.channels + c];

                err_ptr[d * scalars_2d + p * (uint64_t)tex_a.channels + c] = glm::abs(vec_a[c] - vec_b[c]);

                avg_error[c] += glm::abs(vec_a[c] - vec_b[c]) / (double)n;
            }
        }
    }

    spdlog::info("Average Component Error: ({0:.4f}, {1:.4f}, {2:.4f}, {3:.4f})", avg_error.x, avg_error.y, avg_error.z, avg_error.w);
}

void distance_error(const texpress::Texture& tex_a, const texpress::Texture& tex_b, texpress::Texture& tex_error) {
    tex_error.data.clear();
    tex_error.dimensions = tex_a.dimensions;
    tex_error.channels = 1;
    tex_error.gl_internal = texpress::gl_internal(tex_error.channels, 32, true);
    tex_error.gl_format = texpress::gl_format(tex_error.channels);
    tex_error.gl_type = gl::GLenum::GL_FLOAT;
    tex_error.data.resize(tex_a.bytes() / (uint64_t)tex_a.channels);

    double avg_error(0.0);

    const float* a_ptr = (const float*)tex_a.data.data();
    const float* b_ptr = (const float*)tex_b.data.data();
    float* err_ptr = (float*)tex_error.data.data();

    uint64_t vectors_2d = (uint64_t)tex_a.dimensions.x * (uint64_t)tex_a.dimensions.y;
    uint64_t scalars_2d = vectors_2d * (uint64_t)tex_a.channels;

    uint64_t n = vectors_2d * (uint64_t)tex_a.dimensions.z * (uint64_t)tex_a.dimensions.w;

    for (uint64_t d = 0; d < tex_a.dimensions.z * tex_a.dimensions.w; d++) {
        for (uint64_t p = 0; p < tex_a.dimensions.x * tex_a.dimensions.y; p++) {
            glm::vec4 vec_a(0.0f, 0.0f, 0.0f, 1.0f);
            glm::vec4 vec_b(vec_a);

            for (uint64_t c = 0; c < tex_a.channels; c++) {
                vec_a[c] = a_ptr[d * scalars_2d + p * (uint64_t)tex_a.channels + c];
                vec_b[c] = b_ptr[d * scalars_2d + p * (uint64_t)tex_b.channels + c];
            }

            err_ptr[d * vectors_2d + p] = glm::distance(vec_a, vec_b);
            avg_error += glm::distance(vec_a, vec_b) / (double)n;
        }
    }

    spdlog::info("Average Distance Error: {0:.4f}", avg_error);
}

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
        , save_path("")
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
        , abc_x0(-100)
        , abc_y0(0)
        , abc_z0(-100)
        , abc_t0(0)
        , abc_x1(125)
        , abc_y1(250)
        , abc_z1(100)
        , abc_t1(151)
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
        , MaxButtonWidth(0)
    {
        on_prepare = [&]()
            {
                t_curr = clock.timeF64(texpress::WallclockType::WALLCLK_MS);
                t_prev = clock.timeF64(texpress::WallclockType::WALLCLK_MS);
            };
        on_update = [&]()
            {
                t_prev = t_curr;
                t_curr = clock.timeF64(texpress::WallclockType::WALLCLK_MS);

                double t_delta = t_curr - t_prev;
                lag += t_delta;

                while (lag >= MS_PER_UPDATE) {
                    update(MS_PER_UPDATE / 1000.0);
                    lag -= MS_PER_UPDATE;
                }

                MaxButtonWidth = ImGui::CalcTextSize("Denormalize Source").x + 2 * ImGui::GetStyle().ItemInnerSpacing.x;
                // Gui
                // ===
                bool* p_open = NULL;
                ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;

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
                    ImGui::BeginChild("ChildL", { ImGui::GetContentRegionAvail().x * 0.75f, ImGui::GetContentRegionAvail().y * 0.6f }, false, window_flags);
                    if (ImGui::Button("Select HDF5", { MaxButtonWidth, 0 })) {
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
                    if (ImGui::Button("Generate ABC Field", { MaxButtonWidth, 0 })) {
                        uint64_t abc_x, abc_y, abc_z, abc_t;
                        abc_x = abc_x1 - abc_x0;
                        abc_y = abc_y1 - abc_y0;
                        abc_z = abc_z1 - abc_z0;
                        abc_t = abc_t1 - abc_t0;
                        tex_source.channels = 3;
                        tex_source.dimensions = { abc_x, abc_y, abc_z, abc_t };
                        auto buffersize = uint64_t(abc_x) * uint64_t(abc_y) * uint64_t(abc_z) * uint64_t(abc_t) * uint64_t(tex_source.channels) * sizeof(float);
                        tex_source.data.resize(buffersize);
                        tex_source.gl_internal = texpress::gl_internal(tex_source.channels, 32, true);
                        tex_source.gl_format = texpress::gl_format(tex_source.channels);
                        tex_source.gl_type = gl::GLenum::GL_FLOAT;
                        tex_in = &tex_source;

                        const float A_PARAM = glm::sqrt(3);
                        const float B_PARAM = glm::sqrt(2);
                        const float C_PARAM = 1.0;
                        const float c_pos = 0.05;
                        const float c_t1 = 0.05;
                        const float c_t2 = 0.01;

                        glm::vec3* ptr = (glm::vec3*)tex_source.data.data();
                        uint64_t offset = 0;
                        for (auto t = abc_t0; t < abc_t1; t++) {
                            float time = t;
                            float a_coeff = A_PARAM + c_t1 * time * glm::sin(PI * time * c_t2);

                            for (auto z = abc_z0; z < abc_z1; z++) {
                                for (auto y = abc_y0; y < abc_y1; y++) {
                                    for (auto x = abc_x0; x < abc_x1; x++) {
                                        glm::vec3 pos(x, y, z);
                                        glm::vec3 velo(a_coeff * glm::sin(pos.z * c_pos) + B_PARAM * glm::cos(pos.y * c_pos),
                                            B_PARAM * glm::sin(pos.x * c_pos) + C_PARAM * glm::cos(pos.z * c_pos),
                                            C_PARAM * glm::sin(pos.y * c_pos) + a_coeff * glm::cos(pos.x * c_pos));

                                        ptr[offset] = velo;

                                        offset++;
                                    }
                                }
                            }
                        }

                        configuration_changed = false;
                    }
                    ImGui::SameLine();
                    ImGui::Text("[X x Y x Z x T]");
                    const float item_width = 50.0;
                    ImGui::SameLine();
                    ImGui::BeginGroup();
                    ImGui::SetNextItemWidth(item_width);
                    if (ImGui::InputInt("##ABC X0", &abc_x0, 0, 500)) {
                        configuration_changed = true;
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(item_width);
                    if (ImGui::InputInt("##ABC Y0", &abc_y0, 0, 500)) {
                        configuration_changed = true;
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(item_width);
                    if (ImGui::InputInt("##ABC Z0", &abc_z0, 0, 500)) {
                        configuration_changed = true;
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(item_width);
                    if (ImGui::InputInt("##ABC T0", &abc_t0, 0, 500)) {
                        configuration_changed = true;
                    }
                    ImGui::SetNextItemWidth(item_width);
                    if (ImGui::InputInt("##ABC X1", &abc_x1, 0, 500)) {
                        configuration_changed = true;
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(item_width);
                    if (ImGui::InputInt("##ABC Y1", &abc_y1, 0, 500)) {
                        configuration_changed = true;
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(item_width);
                    if (ImGui::InputInt("##ABC Z1", &abc_z1, 0, 500)) {
                        configuration_changed = true;
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(item_width);
                    if (ImGui::InputInt("##ABC T1", &abc_t1, 0, 500)) {
                        configuration_changed = true;
                    }
                    ImGui::EndGroup();

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
                    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
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
                    ImGui::PopItemWidth();
                    ImGui::EndGroup();

                    if (ImGui::Button("Upload HDF5", { MaxButtonWidth, 0 }) && std::filesystem::exists(buf_path)) {
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

                    static int normalize_mode = 0;
                    if (ImGui::Button("Normalize Source", { MaxButtonWidth, 0 })) {
                        tex_normalized.channels = tex_source.channels;
                        tex_normalized.dimensions = tex_source.dimensions;
                        tex_normalized.gl_format = tex_source.gl_format;
                        tex_normalized.gl_internal = tex_source.gl_internal;
                        tex_normalized.gl_type = gl::GLenum::GL_FLOAT;
                        tex_normalized.data.resize(tex_source.bytes());
                        peaks = texpress::find_peaks_per_component<float>(tex_source);

                        if (normalize_mode == 0) {
                            /*
                            for (auto& p : peaks) {
                              uint16_t shifted = fp16_ieee_from_fp32_value(p) & 0b1111111111111110;
                              p = fp16_ieee_to_fp32_value(shifted);
                            }
                            */
                            float* src_ptr = (float*)tex_source.data.data();
                            float* norm_ptr = (float*)tex_normalized.data.data();
                            uint64_t id = 0;
                            for (uint64_t d = 0; d < tex_normalized.dimensions.z * tex_normalized.dimensions.w; d++) {
                                for (uint64_t p = 0; p < tex_normalized.dimensions.x * tex_normalized.dimensions.y; p++) {
                                    for (int c = 0; c < tex_normalized.channels; c++) {
                                        //uint16_t shifted = fp16_ieee_from_fp32_value(src_ptr[id]) & 0b1111111111111110;
                                        //norm_ptr[id] = texpress::normalize_val_per_component(fp16_ieee_to_fp32_value(shifted), tex_source.channels, peaks, d, c);
                                        norm_ptr[id] = texpress::normalize_val_per_component(src_ptr[id], tex_source.channels, peaks, d, c);
                                        id++;
                                    }
                                }
                            }
                        }
                        else {
                            glm::vec3* peak_ptr = (glm::vec3*)peaks.data();
                            min = glm::vec3(INFINITY);
                            max = glm::vec3(-INFINITY);

                            for (uint32_t i = 0; i < peaks.size() / (3 * 2); i += 2) {
                                const auto& p_min = peak_ptr[i];
                                const auto& p_max = peak_ptr[i + 1];
                                min.x = (min.x <= p_min.x) ? min.x : p_min.x;
                                min.y = (min.y <= p_min.y) ? min.y : p_min.y;
                                min.z = (min.z <= p_min.z) ? min.z : p_min.z;

                                max.x = (max.x >= p_max.x) ? max.x : p_max.x;
                                max.y = (max.y >= p_max.y) ? max.y : p_max.y;
                                max.z = (max.z >= p_max.z) ? max.z : p_max.z;
                            }

                            float* src_ptr = (float*)tex_source.data.data();
                            float* norm_ptr = (float*)tex_normalized.data.data();
                            uint64_t id = 0;
                            for (uint64_t d = 0; d < tex_normalized.dimensions.z * tex_normalized.dimensions.w; d++) {
                                for (uint64_t p = 0; p < tex_normalized.dimensions.x * tex_normalized.dimensions.y; p++) {
                                    for (int c = 0; c < tex_normalized.channels; c++) {
                                        //uint16_t shifted = fp16_ieee_from_fp32_value(src_ptr[id]) & 0b1111111111111110;
                                        //norm_ptr[id] = texpress::normalize_val_per_component(fp16_ieee_to_fp32_value(shifted), tex_source.channels, peaks, d, c);
                                        norm_ptr[id] = texpress::normalize_val_per_volume(src_ptr[id], tex_source.channels, min, max, c);
                                        id++;
                                    }
                                }
                            }

                        }
                        tex_out = &tex_normalized;
                    }
                    //ImGui::SameLine();
                    //ImGui::RadioButton("Slice based##norm", &normalize_mode, 0);
                    //ImGui::SameLine();
                    //ImGui::RadioButton("Volume based##norm", &normalize_mode, 1);

                    if (ImGui::Button("Denormalize Source", { MaxButtonWidth, 0 }) && !tex_normalized.data.empty()) {
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
                                    if (normalize_mode == 0) {
                                        dec_ptr[id] = texpress::denormalize_per_component(norm_ptr[id], tex_normalized.channels, peaks, d, c);
                                    }
                                    else {
                                        dec_ptr[id] = texpress::denormalize_per_volume(norm_ptr[id], tex_normalized.channels, min, max, c);
                                    }
                                    id++;
                                }
                            }
                        }

                        tex_out = &tex_decoded;
                    }
                    //ImGui::SameLine();
                    //ImGui::RadioButton("Slice based##denorm_src", &normalize_mode, 0);
                    //ImGui::SameLine();
                    //ImGui::RadioButton("Volume based##denorm_src", &normalize_mode, 1);


                    static const std::vector<char*> compress_options = { "Fast", "Medium", "Production", "Highest" };
                    static int compress_selected = 0;

                    static bool compress_normalized = false;
                    if (ImGui::Button("Compress BC6H", { MaxButtonWidth, 0 }) && (!tex_source.data.empty() || !tex_normalized.data.empty())) {
                        texpress::EncoderSettings settings{};
                        settings.use_weights = false;

                        if (peaks.empty()) {
                            compress_normalized = false;
                        }

                        if (compress_normalized) {
                            //settings.encoding = nvtt::Format::Format_BC6S;
                            settings.encoding = nvtt::Format::Format_BC6U;
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

                        auto t0 = std::chrono::high_resolution_clock::now();
                        if (encoder->compress(settings, input, output)) {
                            spdlog::info("Compressed!");
                            texpress::Encoder::populate_Texture(tex_encoded, output);
                        }
                        auto t1 = std::chrono::high_resolution_clock::now();
                        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
                        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(t1 - t0).count();
                        std::string time = std::to_string(milliseconds) + "\n" + std::to_string(seconds);
                        texpress::file_save("times.txt", time.data(), time.size(), texpress::FILE_TEXT);
                        tex_out = &tex_encoded;
                    }

                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(128);
                    ImGui::Combo("Quality", &compress_selected, compress_options.data(), compress_options.size());

                    ImGui::SameLine();
                    ImGui::Checkbox("Use Normalized Data", &compress_normalized);

                    static bool decompress_and_denormalize = false;
                    if (ImGui::Button("Decompress BC6H", { MaxButtonWidth, 0 }) && !tex_encoded.data.empty()) {
                        texpress::EncoderData input{};
                        texpress::Encoder::populate_EncoderData(input, tex_encoded);

                        texpress::EncoderData output{};
                        output.data_bytes = encoder->decoded_size(input);
                        tex_decoded.data.resize(output.data_bytes);
                        output.data_ptr = reinterpret_cast<uint8_t*>(tex_decoded.data.data());

                        /*
                        if (encoder->decompress(input, output)) {
                          spdlog::info("Decompressed!");
                        }
                        */

                        for (uint32_t d = 0; d < tex_encoded.dimensions.z * tex_encoded.dimensions.w; d++) {
                            encoder->decompress(input, output, d);
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
                                        if (normalize_mode == 0) {
                                            dec_ptr[id] = texpress::denormalize_per_component(dec_ptr[id], tex_normalized.channels, peaks, d, c);
                                        }
                                        else {
                                            dec_ptr[id] = texpress::denormalize_per_volume(dec_ptr[id], tex_normalized.channels, min, max, c);
                                        }

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
                    if (decompress_and_denormalize) {
                        ImGui::SameLine();
                        ImGui::RadioButton("Slice based##denorm_enc", &normalize_mode, 0);
                        ImGui::SameLine();
                        ImGui::RadioButton("Volume based##denorm_enc", &normalize_mode, 1);
                    }

                    if (ImGui::Button("Save", { MaxButtonWidth, 0 })) {
                        ImGui::SetNextWindowSize(ImGui::GetContentRegionAvail());
                        ImGui::OpenPopup("Save Popup");
                    }

                    if (ImGui::BeginPopupModal("Save Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                        static const std::vector<char*> save_options{ "Source", "Normalized", "Compressed", "Decoded", "Peaks", "Error" };

                        static int save_selected = -1;
                        static int saveMode = 0;

                        if (ImGui::Combo("Select data", &save_selected, save_options.data(), save_options.size())) {
                          if (strlen(buf_path) == 0)
                            strcpy(save_path, "C:\\out\\data");
                          else
                            strcpy(save_path, buf_path);
                        }

                        ImGui::InputText("##Savepath", save_path, 128);

                        static bool array2d = false;
                        static bool monolithic = true;

                        ImGui::RadioButton("KTX", &saveMode, 0); ImGui::SameLine();
                        ImGui::RadioButton("Raw", &saveMode, 1); ImGui::SameLine();
                        ImGui::RadioButton("VTK", &saveMode, 2);

                        {
                            std::string tmp = std::filesystem::path(save_path).replace_extension("").string();

                            if (saveMode == 0)
                                tmp += ".ktx";
                            else if (saveMode == 1)
                                tmp += ".raw";
                            else if (saveMode == 2)
                                tmp += ".vtk";

                            strcpy(save_path, tmp.c_str());
                        }

                        if(saveMode == 0)
                        {
                            ImGui::Text("Info: KTX only correctly supports file sizes <= 4GB.");
                        }
                        else if (saveMode == 1)
                        {
                          ImGui::Text("Info: Dataset dimensions are stored in seperate file as integers as xyzw.");
                        }
                        else if (saveMode == 2)
                        {
                          ImGui::Text("Info: VTK generates a seperate file for each timeslice.");
                        }

                        if (ImGui::Button("Save##popup")) {

                            auto dim_save = std::filesystem::path(save_path).replace_extension("").string() + "_dims.raw";

                            switch (save_selected) {
                            case 0:
                                if (tex_source.data.empty()) { break; }

                                if (saveMode == 1) {
                                    texpress::file_save(dim_save.c_str(), (char*)&tex_source.dimensions.x, sizeof(tex_source.dimensions));
                                    texpress::file_save(save_path, (char*)tex_source.data.data(), tex_source.bytes());
                                }
                                else if (saveMode == 2) {
                                    texpress::save_vtk(save_path, "SourceData", tex_source);
                                }
                                else {
                                    texpress::save_ktx(tex_source, save_path, array2d, monolithic);
                                }
                                break;
                            case 1:
                                if (tex_normalized.data.empty()) { break; }

                                if (saveMode == 1) {
                                    texpress::file_save(dim_save.c_str(), (char*)&tex_normalized.dimensions.x, sizeof(tex_normalized.dimensions));
                                    texpress::file_save(save_path, (char*)tex_normalized.data.data(), tex_normalized.bytes());
                                }
                                else if (saveMode == 2) {
                                  texpress::save_vtk(save_path, "NormalizedData", tex_normalized);
                                }
                                else {
                                    texpress::save_ktx(tex_normalized, save_path, array2d, monolithic);
                                }

                                if (!peaks.empty()) {
                                    std::string peaks_path = std::string(save_path);
                                    peaks_path = peaks_path.substr(0, peaks_path.find_last_of('.')) + ".peaks";
                                    texpress::file_save(peaks_path.c_str(), (char*)peaks.data(), peaks.size() * sizeof(float));
                                }
                                break;
                            case 2:
                                if (tex_encoded.data.empty()) { break; }

                                if (saveMode == 1) {
                                    texpress::file_save(dim_save.c_str(), (char*)&tex_encoded.dimensions.x, sizeof(tex_encoded.dimensions));
                                    texpress::file_save(save_path, (char*)tex_encoded.data.data(), tex_encoded.bytes());
                                }
                                else if (saveMode == 2) {
                                    spdlog::error("No VTK for encoded data");
                                }
                                else {
                                    texpress::save_ktx(tex_encoded, save_path, array2d, monolithic);
                                }

                                if (!peaks.empty()) {
                                    std::string peaks_path = std::string(save_path);
                                    peaks_path = peaks_path.substr(0, peaks_path.find_last_of('.')) + ".peaks";
                                    texpress::file_save(peaks_path.c_str(), (char*)peaks.data(), peaks.size() * sizeof(float));
                                }
                                break;
                            case 3:
                                if (tex_decoded.data.empty()) { break; }

                                if (saveMode == 1) {
                                    texpress::file_save(dim_save.c_str(), (char*)&tex_decoded.dimensions.x, sizeof(tex_decoded.dimensions));
                                    texpress::file_save(save_path, (char*)tex_decoded.data.data(), tex_decoded.bytes());
                                }
                                else if (saveMode == 2) {
                                  texpress::save_vtk(save_path, "DecodedData", tex_decoded);
                                }
                                else {
                                    texpress::save_ktx(tex_decoded, save_path, array2d, monolithic);
                                }
                                break;
                            case 4:
                                if (peaks.empty()) { break; }
                                if (saveMode == 1) {
                                    std::string peaks_path = std::string(save_path);
                                    peaks_path = peaks_path.substr(0, peaks_path.find_last_of('.')) + ".peaks";
                                    texpress::file_save(peaks_path.c_str(), (char*)peaks.data(), peaks.size() * sizeof(float));
                                }
                                else if (saveMode == 2) {
                                    spdlog::error("No VTK for peaks data");
                                }
                                else {
                                    spdlog::error("No KTX for peaks data");
                                }
                                break;
                            case 5:
                                if (tex_error.data.empty()) { break; }

                                if (saveMode == 1) {
                                    texpress::file_save(dim_save.c_str(), (char*)&tex_error.dimensions.x, sizeof(tex_error.dimensions));
                                    texpress::file_save(save_path, (char*)tex_error.data.data(), tex_error.bytes());
                                }
                                else if (saveMode == 2) {
                                    texpress::save_vtk(save_path, "ErrorData", tex_error);
                                }
                                else {
                                    texpress::save_ktx(tex_error, save_path, array2d, monolithic);
                                }
                                break;
                            }
                        } ImGui::SameLine();


                        if (ImGui::Button("Close"))
                            ImGui::CloseCurrentPopup();

                        ImGui::EndPopup();
                    }

                    if (ImGui::Button("Load", { MaxButtonWidth, 0 })) {
                        ImGui::SetNextWindowSize(ImGui::GetContentRegionAvail());
                        ImGui::OpenPopup("Load Popup");
                    }

                    if (ImGui::BeginPopupModal("Load Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                        static const std::vector<char*> load_options{ "Source", "Normalized", "Peaks", "Compressed", "Decoded", "Error" };

                        static int load_selected = -1;
                        if (ImGui::Combo("Select data##load", &load_selected, load_options.data(), load_options.size())) {
                            if (load_path[0] == '\0') {
                                strcpy(load_path, (std::string(buf_path) + ".ktx").c_str());
                            }
                        }

                        static bool binary = false;
                        ImGui::Checkbox("Raw##2", &binary);

                        ImGui::InputText("##Loadpath", load_path, 128);

                        if (ImGui::Button("Load##Action")) {
                            auto extension = texpress::str_lowercase(std::filesystem::path(load_path).extension().string());
                            bool raw = extension == ".raw";
                            bool vtk = extension == ".vtk";
                            bool ktx = extension == ".ktx";

                            auto path_dims = std::filesystem::path(load_path).replace_extension("").string() + "_dims" + std::filesystem::path(load_path).extension().string();

                            switch (load_selected) {
                            case 0:
                                if (raw) {
                                    texpress::file_read(path_dims.c_str(), (char*)&tex_source.dimensions.x, sizeof(tex_source.dimensions));
                                    tex_source.channels = texpress::file_size(load_path) / ((uint64_t)tex_source.dimensions.x * (uint64_t)tex_source.dimensions.y * (uint64_t)tex_source.dimensions.z * (uint64_t)tex_source.dimensions.w * sizeof(float));
                                    tex_source.gl_internal = texpress::gl_internal(tex_source.channels, 32, true);
                                    tex_source.gl_format = texpress::gl_format(tex_source.channels);
                                    tex_source.gl_type = gl::GLenum::GL_FLOAT;
                                    tex_source.data.resize(texpress::file_size(load_path));
                                    texpress::file_read(load_path, (char*)tex_source.data.data(), tex_source.bytes());
                                    tex_in = &tex_source;
                                }
                                else if (ktx) {
                                    texpress::load_ktx(load_path, tex_source);
                                    tex_in = &tex_source;
                                }

                                break;
                            case 1:
                                if (raw) {
                                    texpress::file_read(path_dims.c_str(), (char*)&tex_normalized.dimensions.x, sizeof(tex_normalized.dimensions));
                                    tex_normalized.channels = texpress::file_size(load_path) / ((uint64_t)tex_normalized.dimensions.x * (uint64_t)tex_normalized.dimensions.y * (uint64_t)tex_normalized.dimensions.z * (uint64_t)tex_normalized.dimensions.w * sizeof(float));
                                    tex_normalized.gl_internal = texpress::gl_internal(tex_normalized.channels, 32, true);
                                    tex_normalized.gl_format = texpress::gl_format(tex_normalized.channels);
                                    tex_normalized.gl_type = gl::GLenum::GL_FLOAT;
                                    tex_normalized.data.resize(texpress::file_size(load_path));
                                    texpress::file_read(load_path, (char*)tex_normalized.data.data(), tex_normalized.bytes());
                                    tex_out = &tex_normalized;
                                }
                                else if (ktx) {
                                    texpress::load_ktx(load_path, tex_normalized);
                                    tex_out = &tex_normalized;
                                }

                                break;
                            case 2:
                                peaks.clear();
                                peaks.resize(texpress::file_size(load_path) / sizeof(float));
                                texpress::file_read(load_path, (char*)peaks.data(), peaks.size() * sizeof(float));
                                break;
                            case 3:
                                if (raw) {
                                    texpress::file_read(path_dims.c_str(), (char*)&tex_encoded.dimensions.x, sizeof(tex_encoded.dimensions));
                                    tex_encoded.channels = 3;
                                    tex_encoded.gl_internal = texpress::gl_internal(tex_encoded.channels, 32, true);
                                    tex_encoded.gl_format = texpress::gl_format(tex_encoded.channels);
                                    tex_encoded.gl_type = gl::GLenum::GL_FLOAT;
                                    tex_encoded.data.resize(texpress::file_size(load_path));
                                    texpress::file_read(load_path, (char*)tex_encoded.data.data(), tex_encoded.bytes());
                                    tex_out = &tex_encoded;
                                }
                                else if (ktx) {
                                    texpress::load_ktx(load_path, tex_encoded);
                                    tex_out = &tex_encoded;
                                }

                                break;
                            case 4:
                                if (raw) {
                                    texpress::file_read(path_dims.c_str(), (char*)&tex_decoded.dimensions.x, sizeof(tex_decoded.dimensions));
                                    tex_decoded.channels = texpress::file_size(load_path) / ((uint64_t)tex_decoded.dimensions.x * (uint64_t)tex_decoded.dimensions.y * (uint64_t)tex_decoded.dimensions.z * (uint64_t)tex_decoded.dimensions.w * sizeof(float));
                                    tex_decoded.gl_internal = texpress::gl_internal(tex_decoded.channels, 32, true);
                                    tex_decoded.gl_format = texpress::gl_format(tex_decoded.channels);
                                    tex_decoded.gl_type = gl::GLenum::GL_FLOAT;
                                    tex_decoded.data.resize(texpress::file_size(load_path));
                                    texpress::file_read(load_path, (char*)tex_decoded.data.data(), tex_decoded.bytes());
                                    tex_out = &tex_decoded;
                                }
                                else if (ktx) {
                                    texpress::load_ktx(load_path, tex_decoded);
                                    tex_out = &tex_decoded;
                                }

                                break;
                            case 5:
                                if (raw) {
                                    texpress::file_read(path_dims.c_str(), (char*)&tex_error.dimensions.x, sizeof(tex_error.dimensions));
                                    tex_error.channels = texpress::file_size(load_path) / ((uint64_t)tex_error.dimensions.x * (uint64_t)tex_error.dimensions.y * (uint64_t)tex_error.dimensions.z * (uint64_t)tex_error.dimensions.w * sizeof(float));
                                    tex_error.gl_internal = texpress::gl_internal(tex_error.channels, 32, true);
                                    tex_error.gl_format = texpress::gl_format(tex_error.channels);
                                    tex_error.gl_type = gl::GLenum::GL_FLOAT;
                                    tex_error.data.resize(texpress::file_size(load_path));
                                    texpress::file_read(load_path, (char*)tex_error.data.data(), tex_error.bytes());
                                    tex_out = &tex_error;
                                }
                                else if (ktx) {
                                    texpress::load_ktx(load_path, tex_error);
                                    tex_out = &tex_error;
                                }

                                break;
                            }
                        } ImGui::SameLine();


                        if (ImGui::Button("Close"))
                            ImGui::CloseCurrentPopup();

                        ImGui::EndPopup();
                    }

                    if (ImGui::Button("Distance Error", { MaxButtonWidth, 0 })) {
                        if (!tex_source.data.empty()) {
                            distance_error(tex_source, tex_decoded, tex_error);
                            tex_out = &tex_error;
                        }
                    }

                    if (ImGui::Button("Component Error", { MaxButtonWidth, 0 })) {
                        if (!tex_source.data.empty()) {
                            component_error(tex_source, tex_decoded, tex_error);
                            tex_out = &tex_error;
                        }
                    }

                    // --> Quit
                    if (ImGui::Button("Quit", { MaxButtonWidth, 0 })) {
                        spdlog::info("Quit!");
                        dispatcher->post(texpress::Event(texpress::EventType::APP_SHUTDOWN));
                    }

                    ImGui::EndChild();
                }

                ImGui::SameLine();

                // Right menu side
                {
                    ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
                    ImGui::BeginChild("ChildR", { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.6f }, false, window_flags);
                    
                    


                    ImGui::Text("HDF5 File Structure");
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
                                    if (ImGui::IsItemHovered())
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
                                    IMGUI_COLOR_HDFOTHER;
                                }
                                ImGui::Text(node->name.c_str());
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip(node->get_string_type().c_str());
                                //ImGui::SameLine();
                                //if (ImGui::Button(("Copy##" + node->get_path()).c_str())) {
                                //    ImGui::LogToClipboard();
                                //    ImGui::LogText(node->get_path().c_str());
                                //    ImGui::LogFinish();
                                //}
                                IMGUI_UNCOLOR;
                            }
                            };
                        visualize(hdf5_structure.root);
                    }

                    ImGui::NewLine();

                    ImGui::Text("Data Buffers");
                    if (ImGui::Button("Delete##source")) {
                        if (tex_in == &tex_source) { tex_in = nullptr; }
                        if (tex_out == &tex_source) { tex_out = nullptr; }
                        tex_source.data.clear();
                        tex_source.data.shrink_to_fit();
                    } ImGui::SameLine();
                    ImGui::Text("Source Field: "); ImGui::SameLine();
                    ImGui::Text((std::to_string(tex_source.bytes() / (1024 * 1024)) + "MB").c_str());

                    if (ImGui::Button("Delete##normalized")) {
                        if (tex_in == &tex_normalized) { tex_in = nullptr; }
                        if (tex_out == &tex_normalized) { tex_out = nullptr; }
                        tex_normalized.data.clear();
                        tex_normalized.data.shrink_to_fit();
                    } ImGui::SameLine();
                    ImGui::Text("Normalized Field: "); ImGui::SameLine();
                    ImGui::Text((std::to_string(tex_normalized.bytes() / (1024 * 1024)) + "MB").c_str());

                    if (ImGui::Button("Delete##encoded")) {
                        if (tex_in == &tex_encoded) { tex_in = nullptr; }
                        if (tex_out == &tex_encoded) { tex_out = nullptr; }
                        tex_encoded.data.clear();
                        tex_encoded.data.shrink_to_fit();
                    } ImGui::SameLine();
                    ImGui::Text("Encoded Field: "); ImGui::SameLine();
                    ImGui::Text((std::to_string(tex_encoded.bytes() / (1024 * 1024)) + "MB").c_str());

                    if (ImGui::Button("Delete##decoded")) {
                        if (tex_in == &tex_decoded) { tex_in = nullptr; }
                        if (tex_out == &tex_decoded) { tex_out = nullptr; }
                        tex_decoded.data.clear();
                        tex_decoded.data.shrink_to_fit();
                    } ImGui::SameLine();
                    ImGui::Text("Decoded Field: "); ImGui::SameLine();
                    ImGui::Text((std::to_string(tex_decoded.bytes() / (1024 * 1024)) + "MB").c_str());


                    if (ImGui::Button("Delete##error")) {
                        if (tex_in == &tex_error) { tex_in = nullptr; }
                        if (tex_out == &tex_error) { tex_out = nullptr; }
                        tex_error.data.clear();
                        tex_error.data.shrink_to_fit();
                    } ImGui::SameLine();
                    ImGui::Text("Error Field: "); ImGui::SameLine();
                    ImGui::Text((std::to_string(tex_error.bytes() / (1024 * 1024)) + "MB").c_str());

                    ImGui::EndChild();
                }

                {
                    ImGui::BeginChild("ChildBL", { ImGui::GetContentRegionAvail().x * 0.5f, ImGui::GetContentRegionAvail().y }, false, window_flags);
                    if (ImGui::CollapsingHeader("Source Data")) {
                        if (gl_tex_in && tex_in) {
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
                            ImGui::Image(texID, ImVec2{ ImGui::GetContentRegionAvail().y * 0.5f, ImGui::GetContentRegionAvail().y * 0.5f }, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0));
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
                    if (ImGui::CollapsingHeader("Out Data")) {
                        if (gl_tex_out && tex_out) {
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
                            ImGui::Image(texID, ImVec2{ ImGui::GetContentRegionAvail().y * 0.5f, ImGui::GetContentRegionAvail().y * 0.5f }, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0));
                            ImGui::SameLine();
                            ImGui::BeginGroup();
                            ImGui::Text("Size: %i MB", (tex_out->bytes() / (1000 * 1000)));
                            ImGui::Text("Dimensions: %ix%ix%ix%i", tex_out->dimensions.x, tex_out->dimensions.y, tex_out->dimensions.z, tex_out->dimensions.w);
                            ImGui::Text("Channels: %i", tex_out->channels);
                            ImGui::Text("Compression Ratio: %i%:1", tex_in->bytes() / std::max(tex_out->bytes(), 1ULL));
                            ImGui::EndGroup();
                        }
                    }

                    ImGui::EndChild();
                }

                ImGui::End();
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
    int abc_x0;
    int abc_y0;
    int abc_z0;
    int abc_t0;
    int abc_x1;
    int abc_y1;
    int abc_z1;
    int abc_t1;
    uint64_t depth_src;
    uint64_t depth_enc;
    bool configuration_changed;
    bool sync_sliders;

    uint64_t image_level;

    std::vector<float> peaks;
    glm::vec3 min;
    glm::vec3 max;

    float MaxButtonWidth;

    // Threads
    std::thread t_encoder;
};

int main() {
    auto application = std::make_unique<texpress::application>();
    auto renderer = application->add_system<texpress::renderer>();
    auto dispatcher = application->add_system<texpress::Dispatcher>();
    auto encoder = application->add_system<texpress::Encoder>();
    ImGui::SetCurrentContext(application->gui());

    renderer->add_render_pass<texpress::render_pass>(texpress::make_clear_pass(application->window()));
    renderer->add_render_pass<texpress::render_pass>(texpress::make_prepare_pass(application->window()));
    renderer->add_render_pass<update_pass>(dispatcher, encoder);
    renderer->add_render_pass<texpress::render_pass>(texpress::make_gui_pass(application->window()));
    renderer->add_render_pass<texpress::render_pass>(texpress::make_swap_pass(application->window()));

    dispatcher->subscribe(texpress::EventType::APP_SHUTDOWN, std::bind(&texpress::application::listener, application.get(), std::placeholders::_1));
    dispatcher->subscribe(texpress::EventType::COMPRESS_BC6H, std::bind(&texpress::Encoder::listener, encoder, std::placeholders::_1));

    application->run();

    ImGui::DestroyContext();
}