#include <texpress/graphics/render_passes/prepare_pass.hpp>

#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <globjects/Framebuffer.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>

namespace texpress
{
render_pass make_prepare_pass(GLFWwindow* window)
{
  return render_pass
  {
    [ ] ()
    {
      // noop
    },
    [=] ()
    {
      glfwPollEvents();

      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();
    }
  };
}
}
