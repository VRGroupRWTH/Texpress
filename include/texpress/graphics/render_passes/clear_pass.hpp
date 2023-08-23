#pragma once

#include <texpress/graphics/render_pass.hpp>


typedef struct GLFWwindow GLFWwindow;

namespace texpress
{
     render_pass make_clear_pass(GLFWwindow* window);
}
