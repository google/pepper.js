// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sstream>

#include "custom_events.h"

namespace event_queue {

// Convert a given modifier to a descriptive string.  Note that the actual
// declared type of modifier in each of the event classes is uint32_t, but it is
// expected to be interpreted as a bitfield of 'or'ed PP_InputEvent_Modifier
// values.
std::string ModifierToString(uint32_t modifier) {
  std::string s;
  if (modifier & kShiftKeyModifier) {
    s += "shift ";
  }
  if (modifier & kControlKeyModifier) {
    s += "ctrl ";
  }
  if (modifier & kAltKeyModifier) {
    s += "alt ";
  }
  if (modifier & kMetaKeyModifer) {
    s += "meta ";
  }
  if (modifier & kKeyPadModifier) {
    s += "keypad ";
  }
  if (modifier & kAutoRepeatModifier) {
    s += "autorepeat ";
  }
  if (modifier & kLeftButtonModifier) {
    s += "left-button-down ";
  }
  if (modifier & kMiddleButtonModifier) {
    s += "middle-button-down ";
  }
  if (modifier & kRightButtonModifier) {
    s += "right-button-down ";
  }
  if (modifier & kCapsLockModifier) {
    s += "caps-lock ";
  }
  if (modifier & kNumLockModifier) {
    s += "num-lock ";
  }
  return s;
}


std::string KeyEvent::ToString() const {
  std::ostringstream stream;
  stream << " Key event:"
         << " modifier:" << string_event_modifiers()
         << " key_code:" << key_code_
         << " time:" << timestamp_
         << " text:" << text_
         << "\n";
  return stream.str();
}

std::string MouseEvent::ToString() const {
  std::ostringstream stream;
  stream << " Mouse event:"
         << " modifier:" << string_event_modifiers()
         << " button:" << MouseButtonToString(mouse_button_)
         << " x:" << x_position_
         << " y:" << y_position_
         << " click_count:" << click_count_
         << " time:" << timestamp_
         << "\n";
  return stream.str();
}

std::string WheelEvent::ToString() const {
  std::ostringstream stream;
  stream << " Wheel event."
         << " modifier:" << string_event_modifiers()
         << " deltax:" << delta_x_
         << " deltay:" << delta_y_
         << " wheel_ticks_x:" << ticks_x_
         << " wheel_ticks_y:" << ticks_y_
         << " scroll_by_page: " << scroll_by_page_
         << " time:" << timestamp_
         << "\n";
  return stream.str();
}

std::string MouseEvent::MouseButtonToString(MouseButton button) const {
  switch (button) {
    case kNone:
      return "None";
    case kLeft:
      return "Left";
    case kMiddle:
      return "Middle";
    case kRight:
      return "Right";
    default:
      std::ostringstream stream;
      stream << "Unrecognized ("
             << static_cast<int32_t>(button)
             << ")";
      return stream.str();
  }
}

std::string TouchEvent::KindToString(Kind kind) const {
  switch (kind) {
    case kNone:
      return "None";
    case kStart:
      return "Start";
    case kMove:
      return "Move";
    case kEnd:
      return "End";
    case kCancel:
      return "Cancel";
    default:
      std::ostringstream stream;
      stream << "Unrecognized ("
             << static_cast<int32_t>(kind)
             << ")";
      return stream.str();
  }
}

void TouchEvent::AddTouch(uint32_t id, float x, float y, float radii_x,
    float radii_y, float angle, float pressure) {
  Touch touch;
  touch.id = id;
  touch.x = x;
  touch.y = y;
  touch.radii_x = radii_x;
  touch.radii_y = radii_y;
  touch.angle = angle;
  touch.pressure = pressure;
  touches.push_back(touch);
}

std::string TouchEvent::ToString() const {
  std::ostringstream stream;
  stream << " Touch event:" << KindToString(kind_)
         << " modifier:" << string_event_modifiers();
  for (size_t i = 0; i < touches.size(); ++i) {
    const Touch& touch = touches[i];
    stream << " x[" << touch.id << "]:" << touch.x
           << " y[" << touch.id << "]:" << touch.y
           << " radii_x[" << touch.id << "]:" << touch.radii_x
           << " radii_y[" << touch.id << "]:" << touch.radii_y
           << " angle[" << touch.id << "]:" << touch.angle
           << " pressure[" << touch.id << "]:" << touch.pressure;
  }
  stream << " time:" << timestamp_ << "\n";
  return stream.str();
}

}  // end namespace

