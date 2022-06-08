#pragma once

#include <functional>

#include <texpress/export.hpp>

namespace texpress
{
struct TEXPRESS_EXPORT system
{
  std::function<void()> on_prepare = [ ] ( ) { };
  std::function<void()> on_update  = [ ] ( ) { };
};
}
