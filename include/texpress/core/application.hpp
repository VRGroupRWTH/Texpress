#pragma once

#include <texpress/core/engine.hpp>
#include <texpress/events/event.hpp>


typedef struct GLFWwindow GLFWwindow;
typedef struct ImGuiContext ImGuiContext;

namespace texpress
{
    class  application : public engine
    {
    public:
        application();
        application(const application& that) = delete;
        application(application&& temp) = delete;
        ~application() = default;
        application& operator=(const application& that) = delete;
        application& operator=(const application&& temp) = delete;

        GLFWwindow* window() const { return window_; }
        ImGuiContext* gui() const { return gui_; }

        void        run();
        void        quit();
        void        listener(const Event& e);
    protected:
        GLFWwindow* window_ = nullptr;
        ImGuiContext* gui_ = nullptr;
    };
}
