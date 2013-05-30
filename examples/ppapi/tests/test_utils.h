// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_TESTS_TEST_UTILS_H_
#define PPAPI_TESTS_TEST_UTILS_H_

#include <string>

#include "ppapi/c/dev/ppb_testing_dev.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/message_loop.h"
#include "ppapi/utility/completion_callback_factory.h"

// Timeout to wait for some action to complete.
extern const int kActionTimeoutMs;

const PPB_Testing_Dev* GetTestingInterface();
std::string ReportError(const char* method, int32_t error);
void PlatformSleep(int duration_ms);
bool GetLocalHostPort(PP_Instance instance, std::string* host, uint16_t* port);

// NestedEvent allows you to run a nested MessageLoop and wait for a particular
// event to complete. For example, you can use it to wait for a callback on a
// PPP interface, which will "Signal" the event and make the loop quit.
// "Wait()" will return immediately if it has already been signalled. Otherwise,
// it will run a nested message loop (using PPB_Testing.RunMessageLoop) and will
// return only after it has been signalled.
// Example:
//  std::string TestFullscreen::TestNormalToFullscreen() {
//    pp::Fullscreen screen_mode(instance);
//    screen_mode.SetFullscreen(true);
//    SimulateUserGesture();
//    // Let DidChangeView run in a nested message loop.
//    nested_event_.Wait();
//    Pass();
//  }
//
//  void TestFullscreen::DidChangeView(const pp::View& view) {
//    nested_event_.Signal();
//  }
class NestedEvent {
 public:
  explicit NestedEvent(PP_Instance instance)
      : instance_(instance), waiting_(false), signalled_(false) {
  }
  // Run a nested message loop and wait until Signal() is called. If Signal()
  // has already been called, return immediately without running a nested loop.
  void Wait();
  // Signal the NestedEvent. If Wait() has been called, quit the message loop.
  void Signal();
  // Reset the NestedEvent so it can be used again.
  void Reset();
 private:
  PP_Instance instance_;
  bool waiting_;
  bool signalled_;
  // Disable copy and assign.
  NestedEvent(const NestedEvent&);
  NestedEvent& operator=(const NestedEvent&);
};

enum CallbackType { PP_REQUIRED, PP_OPTIONAL, PP_BLOCKING };
class TestCompletionCallback {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {}
    virtual void OnCallback(void* user_data, int32_t result) = 0;
  };
  explicit TestCompletionCallback(PP_Instance instance);
  // TODO(dmichael): Remove this constructor.
  TestCompletionCallback(PP_Instance instance, bool force_async);

  TestCompletionCallback(PP_Instance instance, CallbackType callback_type);

  // Sets a Delegate instance. OnCallback() of this instance will be invoked
  // when the completion callback is invoked.
  // The delegate will be reset when Reset() or GetCallback() is called.
  void SetDelegate(Delegate* delegate) { delegate_ = delegate; }

  // Waits for the callback to be called and returns the
  // result. Returns immediately if the callback was previously called
  // and the result wasn't returned (i.e. each result value received
  // by the callback is returned by WaitForResult() once and only
  // once). DEPRECATED: Please use the one below.
  // TODO(dmichael): Remove this one when all the tests are updated.
  int32_t WaitForResult();

  // Wait for a result, given the return from the call which took this callback
  // as a parameter. If |result| is PP_OK_COMPLETIONPENDING, WaitForResult will
  // block until its callback has been invoked (in some cases, this will already
  // have happened, and WaitForCallback can return immediately).
  // For any other values, WaitForResult will simply set its internal "result_"
  // field. To retrieve the final result of the operation (i.e., the result
  // the callback has run, if necessary), call result(). You can call result()
  // as many times as necessary until a new pp::CompletionCallback is retrieved.
  //
  // In some cases, you may want to check that the callback was invoked in the
  // expected way (i.e., if the callback was "Required", then it should be
  // invoked asynchronously). Within the body of a test (where returning a non-
  // empty string indicates test failure), you can use the
  // CHECK_CALLBACK_BEHAVIOR(callback) macro. From within a helper function,
  // you can use failed() and errors().
  //
  // Example usage within a test:
  //  callback.WaitForResult(foo.DoSomething(callback));
  //  CHECK_CALLBACK_BEHAVIOR(callback);
  //  ASSERT_EQ(PP_OK, callback.result());
  //
  // Example usage within a helper function:
  //  void HelperFunction(std::string* error_message) {
  //    callback.WaitForResult(foo.DoSomething(callback));
  //    if (callback.failed())
  //      error_message->assign(callback.errors());
  //  }
  void WaitForResult(int32_t result);

  // Used when you expect to receive either synchronous completion with PP_OK
  // or a PP_ERROR_ABORTED asynchronously.
  //  Example usage:
  //  int32_t result = 0;
  //  {
  //    pp::URLLoader temp(instance_);
  //    result = temp.Open(request, callback);
  //  }
  //  callback.WaitForAbortResult(result);
  //  CHECK_CALLBACK_BEHAVIOR(callback);
  void WaitForAbortResult(int32_t result);

  // Retrieve a pp::CompletionCallback for use in testing. This Reset()s the
  // TestCompletionCallback.
  pp::CompletionCallback GetCallback();

  // TODO(dmichael): Remove run_count when all tests are updated. Most cases
  //                 that use this can simply use CHECK_CALLBACK_BEHAVIOR.
  unsigned run_count() const { return run_count_; }
  // TODO(dmichael): Remove this; tests should use Reset() instead.
  void reset_run_count() { run_count_ = 0; }

  bool failed() { return !errors_.empty(); }
  const std::string& errors() { return errors_; }

  int32_t result() const { return result_; }

  // Reset so that this callback can be used again.
  void Reset();

 protected:
  static void Handler(void* user_data, int32_t result);
  void RunMessageLoop();
  void QuitMessageLoop();

  // Used to check that WaitForResult is only called once for each usage of the
  // callback.
  bool wait_for_result_called_;
  // Indicates whether we have already been invoked.
  bool have_result_;
  // The last result received (or PP_OK_COMPLETIONCALLBACK if none).
  int32_t result_;
  CallbackType callback_type_;
  bool post_quit_task_;
  std::string errors_;
  unsigned run_count_;
  PP_Instance instance_;
  Delegate* delegate_;
  pp::MessageLoop target_loop_;
};

template <typename OutputT>
class TestCompletionCallbackWithOutput : public TestCompletionCallback {
 public:
  explicit TestCompletionCallbackWithOutput(PP_Instance instance) :
    TestCompletionCallback(instance) {
  }

  TestCompletionCallbackWithOutput(PP_Instance instance, bool force_async) :
    TestCompletionCallback(instance, force_async) {
  }

  TestCompletionCallbackWithOutput(PP_Instance instance,
                                   CallbackType callback_type) :
    TestCompletionCallback(instance, callback_type) {
  }

  pp::CompletionCallbackWithOutput<OutputT> GetCallbackWithOutput();
  operator pp::CompletionCallbackWithOutput<OutputT>() {
    return GetCallbackWithOutput();
  }

  const OutputT& output() { return output_storage_.output(); }

  typename pp::CompletionCallbackWithOutput<OutputT>::OutputStorageType
      output_storage_;
};

template <typename OutputT>
pp::CompletionCallbackWithOutput<OutputT>
TestCompletionCallbackWithOutput<OutputT>::GetCallbackWithOutput() {
  Reset();
  if (callback_type_ == PP_BLOCKING) {
    pp::CompletionCallbackWithOutput<OutputT> cc(
        &TestCompletionCallback::Handler,
        this,
        &output_storage_);
    return cc;
  }

  target_loop_ = pp::MessageLoop::GetCurrent();
  pp::CompletionCallbackWithOutput<OutputT> cc(
        &TestCompletionCallback::Handler,
        this,
        &output_storage_);
  if (callback_type_ == PP_OPTIONAL)
    cc.set_flags(PP_COMPLETIONCALLBACK_FLAG_OPTIONAL);
  return cc;
}


// Verifies that the callback didn't record any errors. If the callback is run
// in an unexpected way (e.g., if it's invoked asynchronously when the call
// should have blocked), this returns an appropriate error string.
#define CHECK_CALLBACK_BEHAVIOR(callback) \
do { \
  if ((callback).failed()) \
    return (callback).errors(); \
} while (false)

/*
 * A set of macros to use for platform detection. These were largely copied
 * from chromium's build_config.h.
 */
#if defined(__APPLE__)
#define PPAPI_OS_MACOSX 1
#elif defined(ANDROID)
#define PPAPI_OS_ANDROID 1
#elif defined(__native_client__)
#define PPAPI_OS_NACL 1
#elif defined(__linux__)
#define PPAPI_OS_LINUX 1
#elif defined(_WIN32)
#define PPAPI_OS_WIN 1
#elif defined(__FreeBSD__)
#define PPAPI_OS_FREEBSD 1
#elif defined(__OpenBSD__)
#define PPAPI_OS_OPENBSD 1
#elif defined(__sun)
#define PPAPI_OS_SOLARIS 1
#else
#error Please add support for your platform in ppapi/tests/test_utils.h
#endif

/* These are used to determine POSIX-like implementations vs Windows. */
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || \
    defined(__OpenBSD__) || defined(__sun) || defined(__native_client__)
#define PPAPI_POSIX 1
#endif

#endif  // PPAPI_TESTS_TEST_UTILS_H_
