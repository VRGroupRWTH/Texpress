#include <texpress/core/application.hpp>

#include <cstdint>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>

#include <glbinding/glbinding.h>
#include <globjects/globjects.h>
#include <GLFW/glfw3.h>

#include <texpress/graphics/renderer.hpp>

namespace texpress
{
application::application()
{
  // Initialize GLFW.
  glfwInit      ();
  const char* glsl_version = "#version 450";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE       , GLFW_OPENGL_CORE_PROFILE);

  // Create window.
  window_ = glfwCreateWindow(1024, 768, "texpress", nullptr, nullptr);
  glfwSetKeyCallback(window_, [ ] (GLFWwindow* window, std::int32_t key, std::int32_t scancode, std::int32_t action, std::int32_t mods)
  {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
      glfwSetWindowShouldClose(window, GLFW_TRUE);
  });

  // Initialize OpenGL.
  glfwMakeContextCurrent(window_);
  glfwSwapInterval      (1);
  glbinding::initialize (glfwGetProcAddress);
  globjects::init       (glfwGetProcAddress);
  globjects::Buffer     ::hintBindlessImplementation (globjects::Buffer ::BindlessImplementation::DirectStateAccessEXT);
  globjects::Texture    ::hintBindlessImplementation (globjects::Texture::BindlessImplementation::DirectStateAccessEXT);
  globjects::VertexArray::hintAttributeImplementation(globjects::VertexArray::AttributeImplementation::DirectStateAccessARB);

  // Initialize ImGui context
  IMGUI_CHECKVERSION();
  gui_ = ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  
  // Setup ImGui
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window_, true);
  ImGui_ImplOpenGL3_Init(glsl_version);
  
  // Create renderer.
  add_system<renderer>();
}
  
void application::run()
{
  for (auto& system : systems_)
    system->on_prepare();
  while (!glfwWindowShouldClose(window_))
    for (auto& system : systems_)
      system->on_update();
}
}
