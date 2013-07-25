/* Copyright (c) 2011 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef PPAPI_TESTS_PP_THREAD_H_
#define PPAPI_TESTS_PP_THREAD_H_

#include "ppapi/c/pp_macros.h"
#include "ppapi/tests/test_utils.h"

#if defined(PPAPI_POSIX)
#include <pthread.h>
#elif defined(PPAPI_OS_WIN)
#include <process.h>
#include <windows.h>
#else
#error No thread library detected.
#endif

/**
 * @file
 * This file provides platform-independent wrappers around threads. This is for
 * use by PPAPI wrappers and tests which need to run on multiple platforms to
 * support both trusted platforms (Windows, Mac, Linux) and untrusted (Native
 * Client). Apps that use PPAPI only with Native Client should generally use the
 * Native Client POSIX implementation instead.
 *
 * TODO(dmichael): Move this file to ppapi/c and delete this comment, if we end
 * up needing platform independent threads in PPAPI C or C++. This file was
 * written using inline functions and PPAPI naming conventions with the intent
 * of making it possible to put it in to ppapi/c. Currently, however, it's only
 * used in ppapi/tests, so is not part of the published API.
 */

#if defined(PPAPI_POSIX)
typedef pthread_t PP_ThreadType;
#elif defined(PPAPI_OS_WIN)
typedef uintptr_t PP_ThreadType;
#endif

typedef void (PP_ThreadFunction)(void* data);

PP_INLINE bool PP_CreateThread(PP_ThreadType* thread,
                               PP_ThreadFunction function,
                               void* thread_arg);
PP_INLINE void PP_JoinThread(PP_ThreadType thread);

#if defined(PPAPI_POSIX)
/* Because POSIX thread functions return void* and Windows thread functions do
 * not, we make PPAPI thread functions have the least capability (no returns).
 * This struct wraps the user data & function so that we can use the correct
 * function type on POSIX platforms.
 */
struct PP_ThreadFunctionArgWrapper {
  void* user_data;
  PP_ThreadFunction* user_function;
};

PP_INLINE void* PP_POSIXThreadFunctionThunk(void* posix_thread_arg) {
  PP_ThreadFunctionArgWrapper* arg_wrapper =
      (PP_ThreadFunctionArgWrapper*)posix_thread_arg;
  arg_wrapper->user_function(arg_wrapper->user_data);
  free(posix_thread_arg);
  return NULL;
}

PP_INLINE bool PP_CreateThread(PP_ThreadType* thread,
                               PP_ThreadFunction function,
                               void* thread_arg) {
  PP_ThreadFunctionArgWrapper* arg_wrapper =
      (PP_ThreadFunctionArgWrapper*)malloc(sizeof(PP_ThreadFunctionArgWrapper));
  arg_wrapper->user_function = function;
  arg_wrapper->user_data = thread_arg;
  return (pthread_create(thread,
                         NULL,
                         PP_POSIXThreadFunctionThunk,
                         arg_wrapper) == 0);
}

PP_INLINE void PP_JoinThread(PP_ThreadType thread) {
  void* exit_status;
  pthread_join(thread, &exit_status);
}

#elif defined(PPAPI_OS_WIN)
typedef DWORD (PP_WindowsThreadFunction)(void* data);

PP_INLINE bool PP_CreateThread(PP_ThreadType* thread,
                               PP_ThreadFunction function,
                               void* thread_arg) {
  if (!thread)
    return false;
  *thread = ::_beginthread(function,
                           0,  /* Use default stack size. */
                           thread_arg);
  return (*thread != NULL);
}

PP_INLINE void PP_JoinThread(PP_ThreadType thread) {
  ::WaitForSingleObject((HANDLE)thread, INFINITE);
}

#endif


/**
 * @}
 */

#endif  /* PPAPI_TESTS_PP_THREAD_H_ */

