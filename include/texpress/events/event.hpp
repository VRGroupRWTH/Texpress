#pragma once

#include <string>

namespace texpress {
  /**
  @class Event
  @brief Base class for all events
  This class provides the interface that needs to be implemented by an event.
  Each dependent class is required to implement type() to uniquely identify
  events. As a convenience, each class should have a static descriptor so that
  clients may refer to it without having to create an instance.
  @author https://github.com/Pseudomanifold/Events
  */

  template <typename T>
  class Event {
  protected:
    T _type;
    std::string _name;
    bool _handled = false;

  public:
    Event() = default;
    Event(T type, const std::string& name = "") : _type(type), _name(name) {};
    virtual ~Event();

    inline const T type() const { return _type; };
    inline const std::string& getName() const { return _name; };
    virtual bool isHandled() { return _handled; };
  };
}