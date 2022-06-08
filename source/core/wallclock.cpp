#include <texpress/defines.hpp>
#include <texpress/core/wallclock.hpp>

#ifdef TEXPRESS_GLFW_CLOCK
  #include <GLFW/glfw3.h>
#else
  #include <chrono>
#endif

namespace texpress
{
#ifdef TEXPRESS_GLFW_CLOCK
#define S 1.0
#define MS 1000.0
#define NS 1000000000.0
  double Wallclock::timeF64(WallclockType type)
  {
    double t_s = glfwGetTime();
    
    switch (type)
    {
    case WallclockType::WALLCLK_S:
      return t_s;

    case WallclockType::WALLCLK_MS:
      return t_s * MS;

    case WallclockType::WALLCLK_NS:
      return t_s * NS;
    }
  }

  uint64_t Wallclock::timeU64(WallclockType type)
  {
    double t_s = glfwGetTime();

    switch (type)
    {
    case WallclockType::WALLCLK_S:
      return t_s;

    case WallclockType::WALLCLK_MS:
      return t_s * MS;

    case WallclockType::WALLCLK_NS:
      return t_s * NS;
    }
  }
#else TEXPRESS_GLFW_TIMER
  typedef std::chrono::high_resolution_clock Time;
  typedef std::chrono::seconds Sec;
  typedef std::chrono::milliseconds Ms;
  typedef std::chrono::nanoseconds Ns;
  typedef std::chrono::duration<double> F64Dur;

  double Wallclock::timeF64(WallclockType type)
  {
    F64Dur t = Time::now().time_since_epoch();

    switch (type)
    {
    case WallclockType::WALLCLK_S:
      F64Dur t_s = t;
      return t_s.count();

    case WallclockType::WALLCLK_MS:
      F64Dur t_ms = std::chrono::duration_cast<Ms>(t);
      return t_ms.count();

    case WallclockType::WALLCLK_NS:
      F64Dur t_ns = std::chrono::duration_cast<Ns>(t);
      return t_ns.count();
    }
  }

  uint64_t Wallclock::timeU64(WallclockType type)
  {
    auto t = Time::now().time_since_epoch();

    switch (type)
    {
    case WallclockType::WALLCLK_S:
      auto t_s = std::chrono::duration_cast<Sec>(t);
      return t_s.count();

    case WallclockType::WALLCLK_MS:
      auto t_ms = std::chrono::duration_cast<Ms>(t);
      return t_ms.count();

    case WallclockType::WALLCLK_NS:
      auto t_ns = std::chrono::duration_cast<Ns>(t);
      return t_ns.count();
    }
  }
#endif
}
