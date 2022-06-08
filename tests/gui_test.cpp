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
  update_pass()
    : MS_PER_UPDATE(16.6)           // Target: 60 FPS
    , clock(texpress::Wallclock())
    , t_curr(0.0)
    , t_prev(0.0)
    , lag(0.0)
    , buf_path("")
  {
    on_prepare = [&] ( )
    {
      t_curr = clock.timeF64(texpress::WallclockType::WALLCLK_MS);
      t_prev = clock.timeF64(texpress::WallclockType::WALLCLK_MS);

      strcpy(buf_path, std::filesystem::current_path().string().c_str());
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
      if (ImGui::Button("Upload File")) {
        if (texpress::file_exists(buf_path)) {
          spdlog::info("Uploading file...");
          auto fsize = texpress::file_size(buf_path);
          std::vector<char> fbuffer(fsize);
          if (!texpress::file_read(buf_path, fbuffer.data(), fbuffer.size()))
            spdlog::error("File upload error!");
          else
            spdlog::info("Uploaded file!");
        }
        else
          spdlog::warn("File does not exist!");
      }
      ImGui::SameLine();
      ImGui::InputText("Filepath", buf_path, 64);
      // --> Compress BC6H
      if (ImGui::Button("Compress to BC6H")) {
        spdlog::info("Compress to BC6H!");
      }
      // --> Save compressed
      if (ImGui::Button("Save Compressed")) {
        spdlog::info("Saved compressed file!");
      }
      // --> Quit
      if (ImGui::Button("Quit")) {
        spdlog::info("Quit!");
      }

      ImGui::End();
      
      ImGui::ShowDemoWindow();
    };
  }
  
  // Logic
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
  ImGui::SetCurrentContext(application->gui());

  renderer->add_render_pass<texpress::render_pass>(texpress::make_clear_pass(application->window()));
  renderer->add_render_pass<texpress::render_pass>(texpress::make_prepare_pass(application->window()));
  renderer->add_render_pass<update_pass>();
  renderer->add_render_pass<texpress::render_pass>(texpress::make_gui_pass(application->window()));
  renderer->add_render_pass<texpress::render_pass>(texpress::make_swap_pass (application->window()));

  application->run();

  ImGui::DestroyContext();
}