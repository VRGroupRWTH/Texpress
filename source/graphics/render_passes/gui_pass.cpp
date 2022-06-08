#include <texpress/graphics/render_passes/gui_pass.hpp>

#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <globjects/Framebuffer.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>

namespace texpress
{
render_pass make_gui_pass(GLFWwindow* window)
{
  return render_pass
  {
    [ ] ()
    {
      // noop
    },
    [=] ()
    {
      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
  };
}
}
