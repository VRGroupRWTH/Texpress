#pragma once

#include <algorithm>
#include <memory>
#include <type_traits>
#include <vector>

#include <texpress/core/system.hpp>
#include <texpress/graphics/render_pass.hpp>

namespace texpress
{
class TEXPRESS_EXPORT renderer : public system
{
public:
  renderer           ();
  renderer           (const renderer&  that) = delete ;
  renderer           (      renderer&& temp) = delete ;
 ~renderer           ()                      = default;
  renderer& operator=(const renderer&  that) = delete ;
  renderer& operator=(      renderer&& temp) = delete ;
  
  template<typename render_pass_type, typename... render_pass_arguments>
  render_pass_type* add_render_pass           (render_pass_arguments&&... arguments)
  {
    static_assert(std::is_base_of<texpress::render_pass, render_pass_type>::value, "The type does not inherit from render_pass.");
    render_passes_.push_back(std::make_unique<render_pass_type>(arguments...));
    const auto render_pass = render_passes_.back().get();
    return static_cast<render_pass_type*>(render_pass);
  }
  template<typename render_pass_type>
  render_pass_type* render_pass               ()
  {
    static_assert(std::is_base_of<texpress::render_pass, render_pass_type>::value, "The type does not inherit from render_pass.");
    auto iterator = std::find_if(render_passes_.begin(), render_passes_.end(), render_pass_type_predicate<render_pass_type>);
    if  (iterator == render_passes_.end())
      return nullptr;
    return static_cast<render_pass_type*>(iterator->get());
  }
  template<typename render_pass_type>
  void              remove_render_pass        ()
  {
    static_assert(std::is_base_of<texpress::render_pass, render_pass_type>::value, "The type does not inherit from render_pass.");
    auto iterator = std::remove_if(render_passes_.begin(), render_passes_.end(), render_pass_type_predicate<render_pass_type>);
    if  (iterator == render_passes_.end())
      return;
    render_passes_.erase(iterator, render_passes_.end());
  }
  
protected:
  template<typename system_type>
  static bool       render_pass_type_predicate(const std::unique_ptr<texpress::render_pass>& iteratee)
  {
    return typeid(system_type) == typeid(*iteratee.get());
  }

  std::vector<std::unique_ptr<texpress::render_pass>> render_passes_;
};
}
