#pragma once

#include <texpress/export.hpp>
#include <cstdint>

namespace texpress
{
  enum TEXPRESS_EXPORT WallclockType {
    WALLCLK_S = 0,
    WALLCLK_MS,
    WALLCLK_NS,
    WALLCLK_GLFW_S
  };

  class TEXPRESS_EXPORT Wallclock
  {
  public:
    Wallclock() = default;
    Wallclock(const Wallclock& that) = delete;
    Wallclock(      Wallclock&& temp) = delete;
    ~Wallclock() = default;
    Wallclock& operator=(const Wallclock& that) = delete;
    Wallclock& operator=(const Wallclock&& temp) = delete;

    double timeF64(WallclockType type = WallclockType::WALLCLK_S);
    uint64_t timeU64(WallclockType type = WallclockType::WALLCLK_S);
  };
}
