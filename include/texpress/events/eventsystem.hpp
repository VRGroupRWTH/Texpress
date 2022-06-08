#pragma once

#include <texpress/export.hpp>
#include <texpress/core/system.hpp>
#include <texpress/events/event.hpp>

#include <functional>
#include <map>
#include <vector>
#include <algorithm>

namespace texpress {
  /**
  @class Dispatcher
  @brief Dispatches events to observers
  The dispatcher is the central class for event management. It offers clients
  the opportunity to use post() to submit a given event. Said event is then
  passed on to all observers that are listening to it. The dispatcher also
  handles new observers via subscribe() and permits the removal of observers
  via disconnect().
  @author https://github.com/Pseudomanifold/Events
*/

  template<typename T>
  class TEXPRESS_EXPORT Dispatcher {
    using SlotType = std::function<void(const Event<T>&)>;
  
  public:
    /**
     * Subscribe an observer
     * @param type  The type of the function to listen to.
     * @param funct function of the observer
     */
    void subscribe(T type, const SlotType& funct) {
      _observers[type].push_back(funct);
    };

    /**
     * Send the event to all the observers
     * @param event event to post
     */
    void post(Event<T>& event) {
      // Ignore events for which we do not have an observer (yet).
      if (_observers.find(event.type()) == _observers.end())
        return;

      //Loop though all observers. If the event is not handled yet we continue to process it.
      for (auto&& observer : _observers.at(event.type())) {
        if (!event.isHandled()) observer(event);
      }
    }

    /**
     * Enqueue the event and process it later
     * @param event event to enqueue
     */
    void enqueue(Event<T>& event) {
      _event_queue.push_back(event);
    }

    /**
     * Process all enqueued events
     */
    void process_queue() {
      for (auto& event : _event_queue) {
        post(event);
      }

      _event_queue.clear();
    }

  private:
    std::map<T, std::vector<SlotType>> _observers;
    std::vector<Event<T>> _event_queue;
  };

  /*
  class ClassObserver
  {
  public:
    void handle(const Event<EventType>& e)
    {
      if (e.type() == EventType::TEST_EVENT)
      {
        // This demonstrates how to obtain the underlying event type in case a
        // slot is set up to handle multiple events of different types.
        // const DemoEvent& demoEvent = static_cast<const DemoEvent&>( e );
        // std::cout << __PRETTY_FUNCTION__ << ": " << demoEvent.type() << std::endl;
        // std::cout << "Class Event EventType1::TEST_EVENT " << e.getName() << std::endl;
      }
    }
  };
  */
}