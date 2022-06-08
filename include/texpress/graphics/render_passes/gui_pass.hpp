#pragma once

#include <texpress/graphics/render_pass.hpp>
#include <texpress/export.hpp>

typedef struct GLFWwindow GLFWwindow;

namespace texpress
{  
TEXPRESS_EXPORT render_pass make_gui_pass(GLFWwindow* window);
}
