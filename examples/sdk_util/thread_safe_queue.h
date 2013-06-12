// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBRARIES_SDK_UTIL_THREAD_SAFE_QUEUE_H_
#define LIBRARIES_SDK_UTIL_THREAD_SAFE_QUEUE_H_

#include <pthread.h>

#include <list>

#include "sdk_util/auto_lock.h"
#include "sdk_util/macros.h"


// ThreadSafeQueue
//
// A simple template to support multithreaded and optionally blocking access
// to a Queue of object pointers.
//
template<class T> class ThreadSafeQueue {
 public:
  ThreadSafeQueue() {
    pthread_mutex_init(&mutex_, NULL);
    pthread_cond_init(&cond_, NULL);
  }

  ~ThreadSafeQueue() {
    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&cond_);
  }

  void Enqueue(T* item) {
    AutoLock lock(&mutex_);
    list_.push_back(item);

    pthread_cond_signal(&cond_);
  }

  T* Dequeue(bool block) {
    AutoLock lock(&mutex_);

    // If blocking enabled, wait until we queue is non-empty
    if (block) {
      while (list_.empty()) pthread_cond_wait(&cond_, &mutex_);
    }

    if (list_.empty()) return NULL;

    T* item = list_.front();
    list_.pop_front();
    return item;
  }

 private:
  std::list<T*> list_;
  pthread_cond_t  cond_;
  pthread_mutex_t mutex_;
  DISALLOW_COPY_AND_ASSIGN(ThreadSafeQueue);
};

#endif  // LIBRARIES_SDK_UTIL_THREAD_SAFE_QUEUE_H_

