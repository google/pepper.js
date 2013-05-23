// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CUSTOM_EVENTS_H
#define CUSTOM_EVENTS_H

#include <stdint.h>
#include <string>
#include <vector>


namespace event_queue {

// These functions and classes are used to define a non-Pepper set of
// events.  This is typical of what many developers might do, since it
// would be common to convert a Pepper event into some other more
// application-specific type of event (SDL, Qt, etc.).

// Constants used for Event::event_modifers_ (which is an int)
// are given below.  Use powers of 2 so we can use bitwise AND/OR operators.
const uint32_t kShiftKeyModifier = 1 << 0;
const uint32_t kControlKeyModifier = 1 << 1;
const uint32_t kAltKeyModifier = 1 << 2;
const uint32_t kMetaKeyModifer = 1 << 3;
const uint32_t kKeyPadModifier = 1 << 4;
const uint32_t kAutoRepeatModifier = 1 << 5;
const uint32_t kLeftButtonModifier = 1 << 6;
const uint32_t kMiddleButtonModifier = 1 << 7;
const uint32_t kRightButtonModifier = 1 << 8;
const uint32_t kCapsLockModifier = 1 << 9;
const uint32_t kNumLockModifier = 1 << 10;

std::string ModifierToString(uint32_t modifier);

// Abstract base class for an Event -- ToString() is not defined.
// With polymorphism, we can have a collection of Event* and call
// ToString() on each one to be able to display the details of each
// event.
class Event {
 public:
  // Constructor for the base class.
  // |modifiers| is an int that uses bit fields to set specific
  // changes, such as the Alt key, specific button, etc.  See
  // ModifierToString() and constants defined in custom_events.cc.
  explicit Event(uint32_t modifiers)
    : event_modifiers_(modifiers) {}
  uint32_t event_modifiers() const {return event_modifiers_;}
  std::string string_event_modifiers() const {
    return ModifierToString(event_modifiers_);
  }
  // Convert the WheelEvent to a string
  virtual std::string ToString() const = 0;
  virtual ~Event() {}

  private:
   uint32_t event_modifiers_;
};

// Class for a keyboard event.
class KeyEvent : public Event {
 public:
  // KeyEvent Constructor. |modifiers| is passed to Event base class.
  // |keycode| is the ASCII value, |time| is a timestamp,
  // |text| is the value as a string.
  KeyEvent(uint32_t modifiers, uint32_t keycode, double time,
           std::string text) :
      Event(modifiers), key_code_(keycode),
      timestamp_(time), text_(text) {}
  // Convert the WheelEvent to a string
  virtual std::string ToString() const;

 private:
   uint32_t key_code_;
   double timestamp_;
   std::string text_;
};

class MouseEvent : public Event {
 public:
  // Specify a mouse button, with kNone available for initialization.
  enum MouseButton {kNone, kLeft, kMiddle, kRight};

  // MouseEvent Constructor. |modifiers| is passed to Event base class.
  // |button| specifies which button
  // |xpos| and |ypos| give the location,
  // |clicks| is how many times this same |xpos|,|ypos|
  // has been clicked in a row.  |time| is a timestamp,
  MouseEvent(uint32_t modifiers, MouseButton button, uint32_t xpos,
             uint32_t ypos, uint32_t clicks, double time)
      : Event(modifiers), mouse_button_(button),
        x_position_(xpos), y_position_(ypos),
        click_count_(clicks), timestamp_(time) {}
  // Convert the WheelEvent to a string
  virtual std::string ToString() const;

 private:
  MouseButton mouse_button_;
  uint32_t x_position_;
  uint32_t y_position_;
  uint32_t click_count_;
  double timestamp_;

  std::string MouseButtonToString(MouseButton button) const;
};


class WheelEvent : public Event {
 public:
  // WheelEvent Constructor. |modifiers| is passed to Event base class.
  // |xticks| and |yticks| specify number of mouse wheel ticks.
  // |scroll_by_page| indicates if we have scrolled past the current
  // page.  |time| is a timestamp,
  WheelEvent(int modifiers, uint32_t dx, uint32_t dy,
             uint32_t xticks, uint32_t yticks, bool scroll_by_page,
             float time) :
      Event(modifiers), delta_x_(dx), delta_y_(dy),
      ticks_x_(xticks), ticks_y_(yticks),
      scroll_by_page_(scroll_by_page), timestamp_(time) {}
  // Convert the WheelEvent to a string
  virtual std::string ToString() const;

 private:
  uint32_t delta_x_;
  uint32_t delta_y_;
  uint32_t ticks_x_;
  uint32_t ticks_y_;
  bool scroll_by_page_;
  double timestamp_;
};


class TouchEvent : public Event {
 public:
  // The kind of touch event that occurred.
  enum Kind { kNone, kStart, kMove, kEnd, kCancel };

  // TouchEvent constructor.
  TouchEvent(int modifiers, Kind kind, float time) :
      Event(modifiers),
      kind_(kind),
      timestamp_(time) {}
  // Add a changed touch to this touch event.
  void AddTouch(uint32_t id, float x, float y, float radii_x, float radii_y,
      float angle, float pressure);
  // Convert the TouchEvent to a string
  virtual std::string ToString() const;

 private:
  std::string KindToString(Kind kind) const;

  struct Touch {
    uint32_t id;
    float x;
    float y;
    float radii_x;
    float radii_y;
    float angle;
    float pressure;
  };

  Kind kind_;
  std::vector<Touch> touches;
  double timestamp_;
};


}  // end namespace

#endif  // CUSTOM_EVENTS_H
