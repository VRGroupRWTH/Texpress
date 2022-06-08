#include <texpress/graphics/renderer.hpp>

namespace texpress
{
renderer::renderer()
{
  on_prepare = [&] ()
  {
    for (auto& pass : render_passes_)
      pass->on_prepare();
  };
  on_update  = [&] ()
  {
    for (auto& pass : render_passes_)
      pass->on_update ();
  };
}
}
