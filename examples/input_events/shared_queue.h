// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHARED_QUEUE_H
#define SHARED_QUEUE_H

#include <pthread.h>
#include <cassert>
#include <deque>

namespace event_queue {

// This file provides a queue that uses a mutex and condition variable so that
// one thread can put pointers into the queue and another thread can pull items
// out of the queue.

// Specifies whether we want to wait for the queue.
enum QueueWaitingFlag {
  kWait = 0,
  kDontWait
};

// Indicates if we got an item, did not wait, or if the queue was cancelled.
enum QueueGetResult {
  kReturnedItem = 0,
  kDidNotWait = 1,
  kQueueWasCancelled
};

// A simple scoped mutex lock.
// For most cases, pp::AutoLock in "ppapi/utility/threading/lock.h" can be
// used; LockingQueue needs to use the pthread_mutex_t directly in
// pthread_cond_wait so we reimplement a scoped lock here.
class ScopedLock {
 public:
  explicit ScopedLock(pthread_mutex_t* mutex)
      : mutex_(mutex) {
    const int kPthreadMutexSuccess = 0;
    if (pthread_mutex_lock(mutex_) != kPthreadMutexSuccess) {
      mutex_ = NULL;
    }
  }
  ~ScopedLock() {
    if (mutex_ != NULL) {
      pthread_mutex_unlock(mutex_);
    }
  }

 private:
  pthread_mutex_t* mutex_;  // Weak reference, passed in to constructor.

  // Disable copy and assign.
  ScopedLock& operator =(const ScopedLock&);
  ScopedLock(const ScopedLock&);
};

// LockingQueue contains a collection of <T>, such as a collection of
// objects or pointers.  The Push() method is used to add items to the
// queue in a thread-safe manner.  The GetItem() is used to retrieve
// items from the queue in a thread-safe manner.
template <class T>
class LockingQueue {
  public:
    LockingQueue() : quit_(false) {
      int result = pthread_mutex_init(&queue_mutex_, NULL);
      assert(result == 0);
      result = pthread_cond_init(&queue_condition_var_, NULL);
      assert(result == 0);
    }
    ~LockingQueue() {
      pthread_mutex_destroy(&queue_mutex_);
    }

    // The producer (who instantiates the queue) calls this to tell the
    // consumer that the queue is no longer being used.
    void CancelQueue() {
      ScopedLock scoped_mutex(&queue_mutex_);
      quit_ = true;
      // Signal the condition var so that if a thread is waiting in
      // GetItem the thread will wake up and see that the queue has
      // been cancelled.
      //pthread_cond_signal(&queue_condition_var_);
    }

    // The consumer calls this to see if the queue has been cancelled by
    // the producer.  If so, the thread should not call GetItem and may
    // need to terminate -- i.e. in a case where the producer created
    // the consumer thread.
    bool IsCancelled() {
      ScopedLock scoped_mutex(&queue_mutex_);
      return quit_;
    }

    // Grabs the mutex and pushes a new item to the end of the queue if the
    // queue is not full.  Signals the condition variable so that a thread
    // that is waiting will wake up and grab the item.
    void Push(const T& item) {
      ScopedLock scoped_mutex(&queue_mutex_);
      the_queue_.push_back(item);
      //pthread_cond_signal(&queue_condition_var_);
    }

    // Tries to pop the front element from the queue; returns an enum:
    //  kReturnedItem if an item is returned in |item_ptr|,
    //  kDidNotWait if |wait| was kDontWait and the queue was empty,
    //  kQueueWasCancelled if the producer called CancelQueue().
    // If |wait| is kWait, GetItem will wait to return until the queue
    // contains an item (unless the queue is cancelled).
    QueueGetResult GetItem(T* item_ptr, QueueWaitingFlag wait) {
      ScopedLock scoped_mutex(&queue_mutex_);
      // Use a while loop to get an item. If the user does not want to wait,
      // we will exit from the loop anyway, unlocking the mutex.
      // If the user does want to wait, we will wait for pthread_cond_wait,
      // and the while loop will check is_empty_no_locking() one more
      // time so that a spurious wake-up of pthread_cond_wait is handled.
      // If |quit_| has been set, break out of the loop.
      while (!quit_ && is_empty_no_locking()) {
        // If user doesn't want to wait, return...
        if (kDontWait == wait) {
          return kDidNotWait;
        }
        // Wait for signal to occur.
        pthread_cond_wait(&queue_condition_var_, &queue_mutex_);
      }
      // Check to see if quit_ woke us up
      if (quit_) {
        return kQueueWasCancelled;
      }

      // At this point, the queue was either not empty or, if it was empty,
      // we called pthread_cond_wait (which released the mutex, waited for the
      // signal to occur, and then atomically reacquired the mutex).
      // Thus, if we are here, the queue cannot be empty because we either
      // had the mutex and verified it was not empty, or we waited for the
      // producer to put an item in and signal a single thread (us).
      T& item = the_queue_.front();
      *item_ptr = item;
      the_queue_.pop_front();
      return kReturnedItem;
    }

  private:
    std::deque<T> the_queue_;
    bool quit_;
    pthread_mutex_t queue_mutex_;
    pthread_cond_t queue_condition_var_;

    // This is used by methods that already have the lock.
    bool is_empty_no_locking() const {
      return the_queue_.empty();
    }
};

}  // end of unnamed namespace

#endif  // SHARED_QUEUE_H

