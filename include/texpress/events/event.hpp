#pragma once
#include <texpress/export.hpp>
#include <string>

namespace texpress {
  enum TEXPRESS_EXPORT EventType {
    APP_SHUTDOWN,
    COMPRESS_BC6H,
    COMPRESS_SAVE
  };


  /**
  @class Event
  @brief Base class for all events
  This class provides the interface that needs to be implemented by an event.
  Each dependent class is required to implement type() to uniquely identify
  events. As a convenience, each class should have a static descriptor so that
  clients may refer to it without having to create an instance.
  @author https://github.com/Pseudomanifold/Events
  */


  struct TEXPRESS_EXPORT Event
  {
    Event() = delete;
    explicit Event(EventType type, void* sendData = nullptr, void* receiveData = nullptr) : mType(type), mSendData(sendData), mReceiveData(receiveData) {}

    EventType mType;
    void* mSendData;
    void* mReceiveData;
  };
}