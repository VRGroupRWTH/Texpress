#pragma once
#include <texpress/export.hpp>
#include <texpress/core/system.hpp>
#include <texpress/events/event.hpp>
#include <texpress/defines.hpp>

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
  class TEXPRESS_EXPORT Compressor : public system
  {
  public:
    Compressor() = default;
    Compressor(const Compressor& that) = delete;
    Compressor(Compressor&& temp) = delete;
    ~Compressor() = default;
    Compressor& operator=(const Compressor& that) = delete;
    Compressor& operator=(Compressor&& temp) = delete;

  public:
    void listener(const Event& e);

  public:
    bool compress(const std::vector<char>& input, std::vector<char>& output);
  };
}