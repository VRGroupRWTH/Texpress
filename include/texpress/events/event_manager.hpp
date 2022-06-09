#pragma once
#include <texpress/export.hpp>
#include <texpress/events/event.hpp>
#include <texpress/core/system.hpp>
#include <texpress/defines.hpp>

#include <algorithm>
#include <memory>
#include <type_traits>
#include <vector>

#include <unordered_map>
#include <functional>
#include <list>

#include <map>

#define FUNCTIONAL_LISTENER(EventType, Listener) EventType, std::bind(&Listener, this, std::placeholders::_1)

namespace texpress
{
  /***
  * Thanks to
  * https://austinmorlan.com/posts/entity_component_system/
  * & RTG Assignment 2
  *
  * The EventManager is used to generate & optionally also directly resolve simple Events.
  * You can do 2 things:
  *     1. Send functional events via SendFunctionalEvent(..).
  *        These events are resolved the moment, they are issued.
  *        But a call to AddFunctionalListener(..) needs  to be done before, which registers a certain method to this event.
  *
  *     2. Send plain events via SendEvent(..).
  *        These events are NOT handled by the EventManager.
  *        You have to manually get mPendingEvents via GetPendingEvents(), loop through all events and resolve them yourself (i.e. in Game.cc).
  *
  * How to do functional Events:
  *     1. Inside class SomeClass define a function.
  *        void SomeClass::InputListener(Event& event) { // Some Code, maybe even altering variables of SomeClass depending on event }
  *
  *     2. Add this function as a listener at some point of execution.
  *        mEventManager->AddFunctionalListener(FUNCTIONAL_LISTENER(EventType::Banana, SomeClass::InputListener));
  *        Done.
  *
  ***/
  class TEXPRESS_EXPORT Dispatcher : public system
  {
  public:
    Dispatcher() = default;
    Dispatcher(const Dispatcher& that) = delete;
    Dispatcher(Dispatcher&& temp) = delete;
    ~Dispatcher() = default;
    Dispatcher& operator=(const Dispatcher& that) = delete;
    Dispatcher& operator=(Dispatcher&& temp) = delete;

  public:
    using SlotType = std::function<void(const Event&)>;

    // Subscribe an observer to an event type
    // Example: subscribe(texpress::EventType::DEMO_EVENT, std::bind(&ObserverClass::handle, observer, std::placeholders::_1));
    void subscribe(const EventType type, SlotType&& slot);

    // Fire an event
    void post(const Event& event) const;

  private:

    std::map<EventType, std::vector<SlotType>> mObservers;
  };
}