#include <catch2/catch_all.hpp>
#include <texpress/api.hpp>

#include <iostream>
#include <filesystem>
#include <spdlog/spdlog.h>

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
    , imgEncoded()
    , texIn(globjects::Texture::createDefault())
    , texOut(globjects::Texture::createDefault())
  {
    on_prepare = [&] ( )
    {
      t_curr = clock.timeF64(texpress::WallclockType::WALLCLK_MS);
      t_prev = clock.timeF64(texpress::WallclockType::WALLCLK_MS);

      //strcpy(buf_path, std::filesystem::current_path().string().c_str());
      strcpy(buf_path, (std::filesystem::current_path().string() + "\\..\\files\\noise.jpg").c_str());
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
      // --> Upload file
      if (ImGui::Button("Upload Image")) {
        if (texpress::file_exists(buf_path)) {
          spdlog::info("Uploading image_ldr...");

          imgIn = texpress::load_image_hdr(buf_path, 4);
          imgSource = texpress::image_to_texture<float>(imgIn);

          for (int i = 0; i < 1; i++) {
            imgSource.data.insert(imgSource.data.end(), imgSource.data.begin(), imgSource.data.end());
            //for (int p = 0; p < imgSource.dimensions.x * imgSource.dimensions.y * imgSource.channels; p++) {
            //  imgSource.data.push_back(1.0f);
            //}
            imgSource.dimensions.z *= 2;
            imgSource.data_size *= 2;
          }

          spdlog::info("Uploaded image_ldr!");
          texIn->image2D(0, imgSource.gl_internal, imgSource.dimensions, 0, imgSource.channels, imgSource.gl_type, imgSource.data.data());

          /*
          if (!texpress::file_read(buf_path, fbuffer.data(), fbuffer.size()))
            spdlog::error("File upload error!");
          else
            spdlog::info("Uploaded file!");
          */
        }
        else
          spdlog::warn("Image does not exist!");
      }
      ImGui::SameLine();
      ImGui::InputText("Filepath", buf_path, 64);
      // --> Compress BC6H
      if (ImGui::Button("Compress to BC6H")) {
        if (imgIn.data.empty()) {
          spdlog::warn("Image does not exist!");
        }
        else {
          spdlog::info("Compressing BC6H...");
          texpress::EncoderSettings settings{};
          settings.use_weights = false;
          settings.encoding = nvtt::Format::Format_BC6S;
          settings.quality = nvtt::Quality::Quality_Fastest;
          settings.progress_ptr = nullptr;

          texpress::EncoderData input{};
          texpress::Encoder::populate_EncoderData(input, imgSource);
          
          texpress::EncoderData output{};
          output.data_bytes = encoder->encoded_size(settings, input);
          imgEncoded.data.resize(output.data_bytes);
          output.data_ptr = imgEncoded.data.data();

          if (encoder->compress(settings, input, output)) {
            spdlog::info("Compressed!");
          }

          texpress::Encoder::populate_Texture(imgEncoded, output);

          texOut->compressedImage2D(0, imgEncoded.gl_internal, glm::ivec2(imgEncoded.dimensions), 0, imgEncoded.bytes() / imgEncoded.dimensions.z, imgEncoded.data.data());
        }
      }

      if (ImGui::Button("Decompress BC6H") && !imgEncoded.data.empty()) {
        texpress::EncoderData input{};
        texpress::Encoder::populate_EncoderData(input, imgEncoded);

        texpress::EncoderData output{};
        output.data_bytes = encoder->decoded_size(input);
        imgDecoded.data.resize(output.data_bytes);
        output.data_ptr = reinterpret_cast<uint8_t*>(imgDecoded.data.data());

        if (encoder->decompress(input, output)) {
          spdlog::info("Decompressed!");
        }

        texpress::Encoder::populate_Texture(imgDecoded, output);

        texOut->image2D(0, imgDecoded.gl_internal, imgDecoded.dimensions, 0, imgDecoded.gl_format, imgDecoded.gl_type, imgDecoded.data.data());
      }

      if (ImGui::Button("Save Source Image")) {
        texpress::save_hdr("img_test", imgIn);
      }

      if (ImGui::Button("Load Source Image")) {
        imgIn = texpress::load_image_hdr("img_test.hdr");
      }

      if (ImGui::Button("Save Source KTX")) {
        texpress::save_ktx(imgSource, "source_test", imgSource.dimensions.z > 1, imgSource.dimensions.w > 1);
      }

      if (ImGui::Button("Load Source KTX")) {
        imgSource = texpress::load_ktx<float>("source_test.ktx");
        texOut->image2D(0, imgSource.gl_internal, imgSource.dimensions, 0, imgSource.gl_format, imgSource.gl_type, imgSource.data.data());
      }

      if (ImGui::Button("Save Compressed KTX")) {
        texpress::save_ktx(imgEncoded, "enc_test", false, false);
      }

      if (ImGui::Button("Load Compressed KTX")) {
        imgEncoded = texpress::load_ktx<uint8_t>("enc_test.ktx");
        texOut->compressedImage2D(0, imgEncoded.gl_internal, glm::ivec2(imgEncoded.dimensions), 0, imgEncoded.bytes() / imgEncoded.dimensions.z, imgEncoded.data.data());
      }

      if (ImGui::Button("Save Decoded KTX")) {
        texpress::save_ktx(imgDecoded, "dec_test.ktx", imgDecoded.dimensions.z > 1, imgDecoded.dimensions.w > 1);
      }

      if (ImGui::Button("Load Decoded KTX")) {
        imgDecoded = texpress::load_ktx<float>("dec_test.ktx");
        texOut->image2D(0, imgDecoded.gl_internal, imgDecoded.dimensions, 0, imgDecoded.gl_format, imgDecoded.gl_type, imgDecoded.data.data());
      }

      // --> Quit
      if (ImGui::Button("Quit")) {
        spdlog::info("Quit!");
        dispatcher->post(texpress::Event(texpress::EventType::APP_SHUTDOWN));
      }

      ImGui::Text("Input Image");
      if (texIn) {
        ImTextureID texID = ImTextureID(texIn->id());
        ImGui::Image(texID, ImVec2(500, 500), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0));
      }

      ImGui::SameLine(); ImGui::Text("Output Image"); ImGui::SameLine();
      if (texOut) {
        ImTextureID texID = ImTextureID(texOut->id());
        ImGui::Image(texID, ImVec2(500, 500), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0));
      }
      ImGui::End();
      
      //ImGui::ShowDemoWindow();
    };
  }

  // Systems
  texpress::Dispatcher* dispatcher;
  texpress::Encoder* encoder;
  
  // Files
  texpress::Texture<float> imgSource;
  texpress::Texture<uint8_t> imgEncoded;
  texpress::Texture<float> imgDecoded;

  // Debug Image
  texpress::image_hdr imgIn;
  std::unique_ptr<globjects::Texture> texIn;
  std::unique_ptr<globjects::Texture> texOut;

  // UpdateLogic
  double MS_PER_UPDATE;
  texpress::Wallclock clock;
  double t_curr;
  double t_prev;
  double lag;

  // Gui
  char buf_path[64];
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