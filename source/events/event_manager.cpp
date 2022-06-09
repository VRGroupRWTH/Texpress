#include <texpress/events/event_manager.hpp>

void texpress::Dispatcher::subscribe(const EventType descriptor, SlotType&& slot)
{
  mObservers[descriptor].push_back(slot);
}

void texpress::Dispatcher::post(const Event& event) const
{
  auto type = event.mType;

  // Ignore events for which we do not have an observer (yet).
  if (mObservers.find(type) == mObservers.end())
    return;

  auto&& observers = mObservers.at(type);

  for (auto&& observer : observers)
    observer(event);
}
