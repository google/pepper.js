// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/tests/test_console.h"

#include "ppapi/cpp/module.h"
#include "ppapi/tests/testing_instance.h"

REGISTER_TEST_CASE(Console);

TestConsole::TestConsole(TestingInstance* instance)
    : TestCase(instance),
      console_interface_(NULL) {
}

bool TestConsole::Init() {
  console_interface_ = static_cast<const PPB_Console*>(
      pp::Module::Get()->GetBrowserInterface(PPB_CONSOLE_INTERFACE));
  return !!console_interface_;
}

void TestConsole::RunTests(const std::string& filter) {
  RUN_TEST(Smoke, filter);
}

std::string TestConsole::TestSmoke() {
  // This test does not verify the log message actually reaches the console, but
  // it does test that the interface exists and that it can be called without
  // crashing.
  pp::Var source(std::string("somewhere"));
  pp::Var message(std::string("hello, world."));
  console_interface_->Log(instance()->pp_instance(), PP_LOGLEVEL_ERROR,
                          message.pp_var());
  console_interface_->LogWithSource(instance()->pp_instance(), PP_LOGLEVEL_LOG,
                                    source.pp_var(), message.pp_var());
  PASS();
}
