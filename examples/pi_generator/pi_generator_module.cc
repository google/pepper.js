// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ppapi/cpp/module.h>

#include "pi_generator.h"

namespace pi_generator {
// The Module class.  The browser calls the CreateInstance() method to create
// an instance of your NaCl module on the web page.  The browser creates a new
// instance for each <embed> tag with type="application/x-nacl".
class PiGeneratorModule : public pp::Module {
 public:
  PiGeneratorModule() : pp::Module() {}
  virtual ~PiGeneratorModule() {}

  // Create and return a PiGeneratorInstance object.
  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new PiGenerator(instance);
  }
};
}  // namespace pi_generator

// Factory function called by the browser when the module is first loaded.
// The browser keeps a singleton of this module.  It calls the
// CreateInstance() method on the object you return to make instances.  There
// is one instance per <embed> tag on the page.  This is the main binding
// point for your NaCl module with the browser.
namespace pp {
Module* CreateModule() {
  return new pi_generator::PiGeneratorModule();
}
}  // namespace pp
