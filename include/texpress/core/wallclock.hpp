#pragma once


#include <cstdint>

namespace texpress
{
    enum  WallclockType {
        WALLCLK_S = 0,
        WALLCLK_MS,
        WALLCLK_NS,
        WALLCLK_GLFW_S
    };

    class  Wallclock
    {
    public:
        Wallclock() = default;
        Wallclock(const Wallclock& that) = delete;
        Wallclock(Wallclock&& temp) = delete;
        ~Wallclock() = default;
        Wallclock& operator=(const Wallclock& that) = delete;
        Wallclock& operator=(const Wallclock&& temp) = delete;

        double timeF64(WallclockType type = WallclockType::WALLCLK_S);
        uint64_t timeU64(WallclockType type = WallclockType::WALLCLK_S);
    };
}
