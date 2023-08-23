#pragma once

#include <texpress/graphics/render_pass.hpp>


typedef struct GLFWwindow GLFWwindow;

namespace texpress
{
     render_pass make_prepare_pass(GLFWwindow* window);
}
