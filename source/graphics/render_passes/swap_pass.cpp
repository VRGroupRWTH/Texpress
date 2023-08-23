#include <texpress/graphics/render_passes/swap_pass.hpp>

#include <GLFW/glfw3.h>

namespace texpress
{
    render_pass make_swap_pass(GLFWwindow* window)
    {
        return render_pass
        {
          []()
          {

          },
          [=]()
          {
            glfwSwapBuffers(window);
            glfwPollEvents();
          }
        };
    }
}
