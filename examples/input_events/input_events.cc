// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// C headers
#include <cassert>
#include <cstdio>

// C++ headers
#include <sstream>
#include <string>

// PPAPI headers
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/input_event.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/point.h"
#include "ppapi/cpp/var.h"
#include "ppapi/utility/completion_callback_factory.h"

#include "custom_events.h"
#include "shared_queue.h"

#ifdef PostMessage
#undef PostMessage
#endif

namespace event_queue {
const char* const kDidChangeView = "DidChangeView";
const char* const kHandleInputEvent = "DidHandleInputEvent";
const char* const kDidChangeFocus = "DidChangeFocus";
const char* const kHaveFocus = "HaveFocus";
const char* const kDontHaveFocus = "DontHaveFocus";
const char* const kCancelMessage = "CANCEL";

// Convert a pepper inputevent modifier value into a
// custom event modifier.
unsigned int ConvertEventModifier(uint32_t pp_modifier) {
  unsigned int custom_modifier = 0;
  if (pp_modifier & PP_INPUTEVENT_MODIFIER_SHIFTKEY) {
    custom_modifier |= kShiftKeyModifier;
  }
  if (pp_modifier & PP_INPUTEVENT_MODIFIER_CONTROLKEY) {
    custom_modifier |= kControlKeyModifier;
  }
  if (pp_modifier & PP_INPUTEVENT_MODIFIER_ALTKEY) {
    custom_modifier |= kAltKeyModifier;
  }
  if (pp_modifier & PP_INPUTEVENT_MODIFIER_METAKEY) {
    custom_modifier |= kMetaKeyModifer;
  }
  if (pp_modifier & PP_INPUTEVENT_MODIFIER_ISKEYPAD) {
    custom_modifier |= kKeyPadModifier;
  }
  if (pp_modifier & PP_INPUTEVENT_MODIFIER_ISAUTOREPEAT) {
    custom_modifier |= kAutoRepeatModifier;
  }
  if (pp_modifier & PP_INPUTEVENT_MODIFIER_LEFTBUTTONDOWN) {
    custom_modifier |= kLeftButtonModifier;
  }
  if (pp_modifier & PP_INPUTEVENT_MODIFIER_MIDDLEBUTTONDOWN) {
    custom_modifier |= kMiddleButtonModifier;
  }
  if (pp_modifier & PP_INPUTEVENT_MODIFIER_RIGHTBUTTONDOWN) {
    custom_modifier |= kRightButtonModifier;
  }
  if (pp_modifier & PP_INPUTEVENT_MODIFIER_CAPSLOCKKEY) {
    custom_modifier |= kCapsLockModifier;
  }
  if (pp_modifier & PP_INPUTEVENT_MODIFIER_NUMLOCKKEY) {
    custom_modifier |= kNumLockModifier;
  }
  return custom_modifier;
}

class EventInstance : public pp::Instance {
 public:
  explicit EventInstance(PP_Instance instance)
      : pp::Instance(instance),
        event_thread_(NULL),
        callback_factory_(this) {
    RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE | PP_INPUTEVENT_CLASS_WHEEL
        | PP_INPUTEVENT_CLASS_TOUCH);
    RequestFilteringInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD);
  }

  // Not guaranteed to be called in Pepper, but a good idea to cancel the
  // queue and signal to workers to die if it is called.
  virtual ~EventInstance() {
    CancelQueueAndWaitForWorker();
  }

  // Create the 'worker thread'.
  bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
    // Avoid creating thread for singlethreaded case
    return true;

    event_thread_ = new pthread_t;
    pthread_create(event_thread_, NULL, ProcessEventOnWorkerThread, this);
    return true;
  }

  /// Clicking outside of the instance's bounding box
  /// will create a DidChangeFocus event (the NaCl instance is
  /// out of focus). Clicking back inside the instance's
  /// bounding box will create another DidChangeFocus event
  /// (the NaCl instance is back in focus). The default is
  /// that the instance is out of focus.
  void DidChangeFocus(bool focus) {
    PostMessage(pp::Var(kDidChangeFocus));
    if (focus == true) {
      PostMessage(pp::Var(kHaveFocus));
    } else {
      PostMessage(pp::Var(kDontHaveFocus));
    }
  }

  /// Scrolling the mouse wheel causes a DidChangeView event.
  void DidChangeView(const pp::View& view) {
    PostMessage(pp::Var(kDidChangeView));
  }

  /// Called by the browser to handle the postMessage() call in Javascript.
  /// Detects which method is being called from the message contents, and
  /// calls the appropriate function.  Posts the result back to the browser
  /// asynchronously.
  /// @param[in] var_message The message posted by the browser.  The only
  ///     supported message is |kCancelMessage|.  If we receive this, we
  ///     cancel the shared queue.
  virtual void HandleMessage(const pp::Var& var_message) {
    std::string message = var_message.AsString();
    if (kCancelMessage == message) {
      std::string reply = "Received cancel : only Focus events will be "
          "displayed. Worker thread for mouse/wheel/keyboard will exit.";
      PostMessage(pp::Var(reply));
      printf("Calling cancel queue\n");
      CancelQueueAndWaitForWorker();
    }
  }

  // HandleInputEvent operates on the main Pepper thread.  Here we
  // illustrate copying the Pepper input event to our own custom event type.
  // Since we need to use Pepper API calls to convert it, we must do the
  // conversion on the main thread.  Once we have converted it to our own
  // event type, we push that into a thread-safe queue and quickly return.
  // The worker thread can process the custom event and do whatever
  // (possibly slow) things it wants to do without making the browser
  // become unresponsive.
  // We dynamically allocate a sub-class of our custom event (Event)
  // so that the queue can contain an Event*.
  virtual bool HandleInputEvent(const pp::InputEvent& event) {
    Event* event_ptr = NULL;

    switch (event.GetType()) {
      case PP_INPUTEVENT_TYPE_IME_COMPOSITION_START:
      case PP_INPUTEVENT_TYPE_IME_COMPOSITION_UPDATE:
      case PP_INPUTEVENT_TYPE_IME_COMPOSITION_END:
      case PP_INPUTEVENT_TYPE_IME_TEXT:
        // these cases are not handled...fall through below...
      case PP_INPUTEVENT_TYPE_UNDEFINED:
        break;
      case PP_INPUTEVENT_TYPE_MOUSEDOWN:
      case PP_INPUTEVENT_TYPE_MOUSEUP:
      case PP_INPUTEVENT_TYPE_MOUSEMOVE:
      case PP_INPUTEVENT_TYPE_MOUSEENTER:
      case PP_INPUTEVENT_TYPE_MOUSELEAVE:
      case PP_INPUTEVENT_TYPE_CONTEXTMENU:
        {
          pp::MouseInputEvent mouse_event(event);
          PP_InputEvent_MouseButton pp_button = mouse_event.GetButton();
          MouseEvent::MouseButton mouse_button = MouseEvent::kNone;
          switch (pp_button) {
            case PP_INPUTEVENT_MOUSEBUTTON_NONE:
              mouse_button = MouseEvent::kNone;
              break;
            case PP_INPUTEVENT_MOUSEBUTTON_LEFT:
              mouse_button = MouseEvent::kLeft;
              break;
            case PP_INPUTEVENT_MOUSEBUTTON_MIDDLE:
              mouse_button = MouseEvent::kMiddle;
              break;
            case PP_INPUTEVENT_MOUSEBUTTON_RIGHT:
              mouse_button = MouseEvent::kRight;
              break;
          }
          event_ptr = new MouseEvent(
              ConvertEventModifier(mouse_event.GetModifiers()),
              mouse_button, mouse_event.GetPosition().x(),
              mouse_event.GetPosition().y(), mouse_event.GetClickCount(),
              mouse_event.GetTimeStamp());
        }
        break;
      case PP_INPUTEVENT_TYPE_WHEEL:
        {
          pp::WheelInputEvent wheel_event(event);
          event_ptr = new WheelEvent(
              ConvertEventModifier(wheel_event.GetModifiers()),
              wheel_event.GetDelta().x(), wheel_event.GetDelta().y(),
              wheel_event.GetTicks().x(), wheel_event.GetTicks().y(),
              wheel_event.GetScrollByPage(), wheel_event.GetTimeStamp());
        }
        break;
      case PP_INPUTEVENT_TYPE_RAWKEYDOWN:
      case PP_INPUTEVENT_TYPE_KEYDOWN:
      case PP_INPUTEVENT_TYPE_KEYUP:
      case PP_INPUTEVENT_TYPE_CHAR:
        {
          pp::KeyboardInputEvent key_event(event);

          event_ptr = new KeyEvent(
              ConvertEventModifier(key_event.GetModifiers()),
              key_event.GetKeyCode(), key_event.GetTimeStamp(),
              key_event.GetCharacterText().DebugString());
        }
        break;
      case PP_INPUTEVENT_TYPE_TOUCHSTART:
      case PP_INPUTEVENT_TYPE_TOUCHMOVE:
      case PP_INPUTEVENT_TYPE_TOUCHEND:
      case PP_INPUTEVENT_TYPE_TOUCHCANCEL:
        {
          pp::TouchInputEvent touch_event(event);

          TouchEvent::Kind touch_kind = TouchEvent::kNone;
          if (event.GetType() == PP_INPUTEVENT_TYPE_TOUCHSTART)
            touch_kind = TouchEvent::kStart;
          else if (event.GetType() == PP_INPUTEVENT_TYPE_TOUCHMOVE)
            touch_kind = TouchEvent::kMove;
          else if (event.GetType() == PP_INPUTEVENT_TYPE_TOUCHEND)
            touch_kind = TouchEvent::kEnd;
          else if (event.GetType() == PP_INPUTEVENT_TYPE_TOUCHCANCEL)
            touch_kind = TouchEvent::kCancel;

          TouchEvent* touch_event_ptr = new TouchEvent(
              ConvertEventModifier(touch_event.GetModifiers()),
              touch_kind, touch_event.GetTimeStamp());
          event_ptr = touch_event_ptr;

          uint32_t touch_count =
              touch_event.GetTouchCount(PP_TOUCHLIST_TYPE_CHANGEDTOUCHES);
          for (uint32_t i = 0; i < touch_count; ++i) {
            pp::TouchPoint point = touch_event.GetTouchByIndex(
                PP_TOUCHLIST_TYPE_CHANGEDTOUCHES, i);
            touch_event_ptr->AddTouch(point.id(), point.position().x(),
                point.position().y(), point.radii().x(), point.radii().y(),
                point.rotation_angle(), point.pressure());
          }
        }
        break;
      default:
        {
          // For any unhandled events, send a message to the browser
          // so that the user is aware of these and can investigate.
          std::stringstream oss;
          oss << "Default (unhandled) event, type=" << event.GetType();
          PostMessage(oss.str());
        }
        break;
    }

    event_queue_.Push(event_ptr);
    ProcessEventOnWorkerThread(this); //call function worker normally uses to process the queue, but without looping forever
    return true;
  }

  // Return an event from the thread-safe queue, waiting for a new event
  // to occur if the queue is empty.  Set |was_queue_cancelled| to indicate
  // whether the queue was cancelled.  If it was cancelled, then the
  // Event* will be NULL.
  const Event* GetEventFromQueue(bool *was_queue_cancelled) {
    Event* event = NULL;
    // Don't wait on the queue, returned cancled if it is empty
    QueueGetResult result = event_queue_.GetItem(&event, kDontWait);
    if (result == kDidNotWait | result == kQueueWasCancelled) {
      *was_queue_cancelled = true;
      return NULL;
    }
    *was_queue_cancelled = false;
    return event;
  }

  // This method is called from the worker thread using CallOnMainThread.
  // It is not static, and allows PostMessage to be called.
  void* PostStringToBrowser(int32_t result, std::string data_to_send) {
    PostMessage(pp::Var(data_to_send));
    return 0;
  }

  // |ProcessEventOnWorkerThread| is a static method that is run
  // by a thread.  It pulls events from the queue, converts
  // them to a string, and calls CallOnMainThread so that
  // PostStringToBrowser will be called, which will call PostMessage
  // to send the converted event back to the browser.
  static void* ProcessEventOnWorkerThread(void* param) {
    EventInstance* event_instance = static_cast<EventInstance*>(param);
    while (1) {
      // Grab a generic Event* so that down below we can call
      // event->ToString(), which will use the correct virtual method
      // to convert the event to a string.  This 'conversion' is
      // the 'work' being done on the worker thread.  In an application
      // the work might involve changing application state based on
      // the event that was processed.
      bool queue_cancelled;
      const Event* event = event_instance->GetEventFromQueue(&queue_cancelled);
      if (queue_cancelled) {
        // Don't print message and exit since it's not really cancled most likely
        break;

        printf("Queue was cancelled, worker thread exiting\n");
        pthread_exit(NULL);
      }
      std::string event_string = event->ToString();
      delete event;
      // Need to invoke callback on main thread.
      pp::Module::Get()->core()->CallOnMainThread(
          0,
          event_instance->callback_factory().NewCallback(
              &EventInstance::PostStringToBrowser,
              event_string));
    }  // end of while loop.
    return 0;
  }

  // Return the callback factory.
  // Allows the static method (ProcessEventOnWorkerThread) to use
  // the |event_instance| pointer to get the factory.
  pp::CompletionCallbackFactory<EventInstance>& callback_factory() {
    return callback_factory_;
  }

  private:
    // Cancels the queue (which will cause the thread to exit).
    // Wait for the thread.  Set |event_thread_| to NULL so we only
    // execute the body once.
    void CancelQueueAndWaitForWorker() {
      // Cancel queue even if thread is null
      event_queue_.CancelQueue();

      if (event_thread_) {
        pthread_join(*event_thread_, NULL);
        delete event_thread_;
        event_thread_ = NULL;
      }
    }
    pthread_t* event_thread_;
    LockingQueue<Event*> event_queue_;
    pp::CompletionCallbackFactory<EventInstance> callback_factory_;
};

// The EventModule provides an implementation of pp::Module that creates
// EventInstance objects when invoked.  This is part of the glue code that makes
// our example accessible to ppapi.
class EventModule : public pp::Module {
 public:
  EventModule() : pp::Module() {}
  virtual ~EventModule() {}

  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new EventInstance(instance);
  }
};

}  // namespace

// Implement the required pp::CreateModule function that creates our specific
// kind of Module (in this case, EventModule).  This is part of the glue code
// that makes our example accessible to ppapi.
namespace pp {
  Module* CreateModule() {
    return new event_queue::EventModule();
  }
}

