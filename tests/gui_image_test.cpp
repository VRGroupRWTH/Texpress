#include <catch2/catch_all.hpp>
#include <texpress/api.hpp>

#include <iostream>
#include <filesystem>
#include <spdlog/spdlog.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
      strcpy(buf_path, (std::filesystem::current_path().string() + "\\..\\files\\vr.jpg").c_str());
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
          spdlog::info("Uploading image...");

          int x, y, n;
          int c = 4;
          float* image_data = stbi_loadf(buf_path, &x, &y, &n, c);
          if (c == 0)
            c = n;

          imgIn.size = glm::ivec2(x, y);
          imgIn.channels = c;
          imgIn.data.assign(image_data, image_data + x * y * c);
          stbi_image_free(image_data);

          imgIn = texpress::fit_blocksize(glm::ivec2(4, 4), imgIn);

          spdlog::info("Uploaded image!");
          texIn->image2D(0, gl::GL_RGBA32F, imgIn.size, 0, gl::GL_RGBA, gl::GL_FLOAT, imgIn.data.data());

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
          imgEncoded = encoder->compress_bc6h(imgIn);
          spdlog::info("Compressed!");

          texOut->compressedImage2D(0, imgEncoded.enc_glformat, glm::ivec2(imgEncoded.grid_size), 0, imgEncoded.data_size, imgEncoded.data_ptr.data());
        }
      }
      // --> Save compressed
      if (ImGui::Button("Save Compressed")) {
        dispatcher->post(texpress::Event(texpress::EventType::COMPRESS_SAVE));
        spdlog::info("Saved compressed file!");
      }
      // --> Quit
      if (ImGui::Button("Quit")) {
        spdlog::info("Quit!");
        dispatcher->post(texpress::Event(texpress::EventType::APP_SHUTDOWN));
      }

      ImGui::Text("Input Image");
      if (texIn) {
        ImTextureID texID = ImTextureID(texIn->id());
        ImGui::Image(texID, ImVec2(imgIn.size.x, imgIn.size.y), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0));
      }

      ImGui::Text("Output Image");
      if (texOut) {
        ImTextureID texID = ImTextureID(texOut->id());
        ImGui::Image(texID, ImVec2(imgEncoded.grid_size.x, imgEncoded.grid_size.y), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0, 1.0, 1.0, 1.0), ImVec4(1.0, 1.0, 1.0, 1.0));
      }
      ImGui::End();
      
      //ImGui::ShowDemoWindow();
    };
  }

  // Systems
  texpress::Dispatcher* dispatcher;
  texpress::Encoder* encoder;
  
  // Files
  texpress::BlockCompressed imgEncoded;

  // Debug Image
  hdr_image imgIn;
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