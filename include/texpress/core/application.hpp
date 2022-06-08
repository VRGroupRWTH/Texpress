#pragma once

#include <texpress/core/engine.hpp>
#include <texpress/export.hpp>

typedef struct GLFWwindow GLFWwindow;
typedef struct ImGuiContext ImGuiContext;

namespace texpress
{
class TEXPRESS_EXPORT application : public engine
{
public:
  application           ();
  application           (const application&  that) = delete ;
  application           (      application&& temp) = delete ;
 ~application           ()                         = default;
  application& operator=(const application&  that) = delete ;
  application& operator=(const application&& temp) = delete ;
  
  GLFWwindow* window    () const { return window_; }
  ImGuiContext* gui() const { return gui_; }

  void        run       ();
protected:
  GLFWwindow* window_ = nullptr;
  ImGuiContext* gui_ = nullptr;
};
}
