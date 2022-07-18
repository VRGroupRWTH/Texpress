#include <catch2/catch_all.hpp>
#include <texpress/api.hpp>

#include <iostream>
#include <filesystem>
#include <spdlog/spdlog.h>

#define IMGUI_COLOR_HDFGROUP ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(148/255.f, 180/255.f, 159/255.f, 255/255.f))
#define IMGUI_COLOR_HDFDATASET ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(206/255.f, 229/255.f, 208/255.f, 255/255.f))
#define IMGUI_COLOR_HDFOTHER ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(252/255.f, 248/255.f, 232/255.f, 255/255.f))
#define IMGUI_UNCOLOR ImGui::PopStyleColor()


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
    , buf_x("/u")
    , buf_y("/v")
    , buf_z("/w")
    , stride_x(1)
    , stride_y(1)
    , stride_z(1)
    , offset_x(0)
    , offset_y(0)
    , offset_z(0)
    , texIn(globjects::Texture::createDefault())
    , texOut(globjects::Texture::createDefault())
    , hdf5_file(nullptr)
    , hdf5_data()
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
        ImGui::InputText("##Filepath", buf_path, 64);

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
        if (ImGui::InputText("##X Dataset", buf_x, 64))
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
        if (ImGui::InputText("##Y Dataset", buf_y, 64))
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
        if (ImGui::InputText("##Z Dataset", buf_z, 64))
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

        if (ImGui::Button("Compress BC6H")) {
          texpress::hdf5 file(buf_path);
          hdf5_data.data = file.read_dataset<float>({ buf_x, buf_y, buf_z }, { uint64_t(offset_x), uint64_t(offset_y), uint64_t(offset_z) }, { uint64_t(stride_x), uint64_t(stride_y), uint64_t(stride_z) });
          hdf5_data.data = texpress::interleave_force(hdf5_data.data, 4, hdf5_data.data.size() / 3, 0.0f);
          hdf5_data.data_channels = 4;
          hdf5_data.data_size = hdf5_data.data.size();
          hdf5_data.grid_glType = gl::GLenum::GL_FLOAT;
          auto dims = file.dataset_dimensions(buf_x);
          for (int i = 0; i < hdf5_data.grid_size.length(); i++) {
            hdf5_data.grid_size[i] = (i < dims.size()) ? dims[i] : 1;
          }
          hdf5_encoded = encoder->compress_bc6h(texpress::BC6H_options(), hdf5_data);
          hdf5_decoded = encoder->decompress_bc6h(texpress::BC6H_options(), hdf5_encoded);
          spdlog::info("Compress BC6H!");

          configuration_changed = false;
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
        window_flags |= ImGuiWindowFlags_NoScrollWithMouse;
        ImGui::BeginChild("ChildR", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0), false, window_flags);
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
                for (auto* child : node->children)
                  visualize(child);
                ImGui::TreePop();
              }
              else { IMGUI_UNCOLOR; }
            }

            // Leafs
            else if (node->parent) {
              if (node->is_dataset()) { IMGUI_COLOR_HDFDATASET; }
              else {
                IMGUI_COLOR_HDFOTHER; }
              ImGui::Text(node->name.c_str());
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
  texpress::Texture<float> hdf5_data;
  texpress::Texture<uint8_t> hdf5_encoded;
  texpress::Texture<float> hdf5_decoded;
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
  char buf_x[64];
  char buf_y[64];
  char buf_z[64];
  int stride_x;
  int stride_y;
  int stride_z;
  int offset_x;
  int offset_y;
  int offset_z;
  bool configuration_changed;
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