#pragma once

#include <functional>



namespace texpress
{
    struct  system
    {
        std::function<void()> on_prepare = []() {};
        std::function<void()> on_update = []() {};
    };
}
