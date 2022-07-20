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

      // debug
      auto to_argb = [](texpress::Texture<float>& inp) {
        for (auto pixel = 0; pixel < inp.grid_size.z * inp.grid_size.y * inp.grid_size.x; pixel++) {
          auto r = inp.data[pixel * 4 + 0];
          auto g = inp.data[pixel * 4 + 1];
          auto b = inp.data[pixel * 4 + 2];
          auto a = inp.data[pixel * 4 + 3];

          inp.data[pixel * 4 + 0] = a;
          inp.data[pixel * 4 + 1] = r;
          inp.data[pixel * 4 + 2] = g;
          inp.data[pixel * 4 + 3] = b;
        }
      };

      auto to_rgba = [](texpress::Texture<float>& inp) {
        for (auto pixel = 0; pixel < inp.grid_size.z * inp.grid_size.y * inp.grid_size.x; pixel++) {
          auto a = inp.data[pixel * 4 + 0];
          auto r = inp.data[pixel * 4 + 1];
          auto g = inp.data[pixel * 4 + 2];
          auto b = inp.data[pixel * 4 + 3];

          inp.data[pixel * 4 + 0] = r;
          inp.data[pixel * 4 + 1] = g;
          inp.data[pixel * 4 + 2] = b;
          inp.data[pixel * 4 + 3] = a;
        }
      };

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
          spdlog::info("Uploading image...");

          imgIn = texpress::load_image_hdr(buf_path, 4);
          //imgIn = texpress::fit_blocksize(glm::ivec2(4, 4), imgIn);
          imgSource = texpress::image_to_texture<float>(imgIn);

          for (int i = 0; i < 1; i++) {
            imgSource.data.insert(imgSource.data.end(), imgSource.data.begin(), imgSource.data.end());
            //for (int p = 0; p < imgSource.grid_size.x * imgSource.grid_size.y * imgSource.data_channels; p++) {
            //  imgSource.data.push_back(1.0f);
            //}
            imgSource.grid_size.z *= 2;
            imgSource.data_size *= 2;
          }

          spdlog::info("Uploaded image!");
          texIn->image2D(0, imgSource.gl_internalFormat, imgSource.grid_size, 0, imgSource.gl_pixelFormat, imgSource.gl_type, imgSource.data.data());

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
          //to_argb(imgSource);
          imgEncoded = encoder->compress_bc6h(texpress::BC6H_options{ 0.05f, 0, true }, imgSource);
          spdlog::info("Compressed!");

          texOut->compressedImage2D(0, imgEncoded.gl_internalFormat, glm::ivec2(imgEncoded.grid_size), 0, imgEncoded.data_size / imgEncoded.grid_size.z, imgEncoded.data.data());
        }
      }

      if (ImGui::Button("Decompress BC6H") && !imgEncoded.data.empty()) {
        imgDecoded = encoder->decompress_bc6h(texpress::BC6H_options(), imgEncoded);
        //to_rgba(imgDecoded);
        texOut->image2D(0, imgDecoded.gl_internalFormat, imgDecoded.grid_size, 0, imgDecoded.gl_pixelFormat, imgDecoded.gl_type, imgDecoded.data.data());
      }

      if (ImGui::Button("Save Source Image")) {
        texpress::save_hdr("img_test", imgIn);
      }

      if (ImGui::Button("Load Source Image")) {
        imgIn = texpress::load_image_hdr("img_test.hdr");
      }

      if (ImGui::Button("Save Source KTX")) {
        texpress::save_ktx(imgSource, "source_test.ktx", imgSource.grid_size.z > 1, imgSource.grid_size.w > 1);
      }

      if (ImGui::Button("Load Source KTX")) {
        imgSource = texpress::load_ktx<float>("source_test.ktx");
        texOut->image2D(0, imgSource.gl_internalFormat, imgSource.grid_size, 0, imgSource.gl_pixelFormat, imgSource.gl_type, imgSource.data.data());
      }

      if (ImGui::Button("Save Compressed KTX")) {
        texpress::save_ktx(imgEncoded, "enc_test.ktx", imgEncoded.grid_size.z > 1, imgEncoded.grid_size.w > 1);
      }

      if (ImGui::Button("Load Compressed KTX")) {
        imgEncoded = texpress::load_ktx<uint8_t>("enc_test.ktx");
        texOut->compressedImage2D(0, imgEncoded.gl_internalFormat, glm::ivec2(imgEncoded.grid_size), 0, imgEncoded.data_size / imgEncoded.grid_size.z, imgEncoded.data.data());
      }

      if (ImGui::Button("Save Decoded KTX")) {
        texpress::save_ktx(imgDecoded, "dec_test.ktx", imgDecoded.grid_size.z > 1, imgDecoded.grid_size.w > 1);
      }

      if (ImGui::Button("Load Decoded KTX")) {
        imgDecoded = texpress::load_ktx<float>("dec_test.ktx");
        texOut->image2D(0, imgDecoded.gl_internalFormat, imgDecoded.grid_size, 0, imgDecoded.gl_pixelFormat, imgDecoded.gl_type, imgDecoded.data.data());
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
  texpress::hdr_image imgIn;
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