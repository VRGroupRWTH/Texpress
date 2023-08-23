#include <texpress/graphics/render_passes/clear_pass.hpp>

#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <globjects/Framebuffer.h>
#include <GLFW/glfw3.h>

#undef GL_COLOR_BUFFER_BIT
#undef GL_DEPTH_BUFFER_BIT

namespace texpress
{
    render_pass make_clear_pass(GLFWwindow* window)
    {
        return render_pass
        {
          []()
          {
            globjects::Framebuffer::defaultFBO()->clearColor(0.1, 0.1, 0.1, 1.0);
          },
          [=]()
          {
            glm::ivec2 size;
            glfwGetFramebufferSize(window, &size[0], &size[1]);
            gl::glViewport(0, 0,    size[0],  size[1]);
            globjects::Framebuffer::defaultFBO()->clear(gl::ClearBufferMask::GL_COLOR_BUFFER_BIT | gl::ClearBufferMask::GL_DEPTH_BUFFER_BIT);
          }
        };
    }
}
