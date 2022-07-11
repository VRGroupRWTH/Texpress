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
    , texIn(globjects::Texture::createDefault())
    , texOut(globjects::Texture::createDefault())
    , hdf5_file(nullptr)
  {
    on_prepare = [&] ( )
    {
      t_curr = clock.timeF64(texpress::WallclockType::WALLCLK_MS);
      t_prev = clock.timeF64(texpress::WallclockType::WALLCLK_MS);

      //strcpy(buf_path, std::filesystem::current_path().string().c_str());
      strcpy(buf_path, (std::filesystem::current_path().string() + "\\..\\files\\Test.HDF5").c_str());
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
        window_flags |= ImGuiWindowFlags_NoScrollWithMouse;
        //ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 260), false, window_flags);
        ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0), false, window_flags);
        if (ImGui::Button("Upload HDF5")) {
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
        ImGui::InputText("Filepath", buf_path, 64);
        ImGui::Button("Quit");
        ImGui::EndChild();
      }

      ImGui::SameLine();

      // Right menu side
      {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        window_flags |= ImGuiWindowFlags_NoScrollWithMouse;
        ImGui::BeginChild("ChildR", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0), false, window_flags);
        if (!hdf5_structure.empty()) {
          auto node = hdf5_structure.root;

          if (ImGui::TreeNode("HDF5 File")) {
            for (const auto* group : groups) {
              if (ImGui::TreeNode(group->name.c_str())) {
                for (const auto* child : group->children) {
                  if (ImGui::TreeNode(group->name.c_str())) {

                    ImGui::TreePop();
                  }
                }
                ImGui::TreePop();
              }
            }
            
            ImGui::TreePop();
          }

        }
        ImGui::EndChild();
      }

      // --> Quit
      if (ImGui::Button("Quit")) {
        spdlog::info("Quit!");
        dispatcher->post(texpress::Event(texpress::EventType::APP_SHUTDOWN));
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