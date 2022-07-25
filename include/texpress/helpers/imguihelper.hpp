#pragma once
#include <glm/vec2.hpp>

// Credits: https://github.com/ocornut/imgui/issues/1901
namespace ImGui {
  bool BufferingBar(const char* label, float value, const glm::vec2& size_arg, const uint32_t& bg_col, const uint32_t& fg_col);
  bool Spinner(const char* label, float radius, int thickness, const uint32_t& color);
}