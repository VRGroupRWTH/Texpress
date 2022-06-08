#pragma once
#include <texpress/export.hpp>
#include <texpress/events/event.hpp>

namespace texpress {
  enum class EventType {
    SYSTEM_EVENT
  };

  class TEXPRESS_EXPORT TexpressEvent : public Event<EventType>
  {
  public:
    TexpressEvent() : Event<EventType>(EventType::SYSTEM_EVENT, "SystemEvent") {};
    virtual ~TexpressEvent() = default;
  };

}
