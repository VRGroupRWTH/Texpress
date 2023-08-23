#pragma once

#include <functional>



namespace texpress
{
    struct  render_pass
    {
        std::function<void()> on_prepare = []() {};
        std::function<void()> on_update = []() {};
    };
}
