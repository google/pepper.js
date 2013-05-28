// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(DISABLE_THREADS)

#include "threadpool.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

// TODO(nfullagar): Switch DecCounter to use atomic decrement.
// TODO(nfullagar): Large sets of fine grained tasks may benefit from
// pthread_condvar_* over sem_*

// Initializes mutex, semaphores and a pool of threads.
ThreadPool::ThreadPool(const int num_threads)
    : threads_(NULL), counter_(0), num_threads_(num_threads), exiting_(false),
      user_data_(NULL), user_work_function_(NULL) {
  if (num_threads_ > 1) {
    int status;
    status = pthread_mutex_init(&mutex_, NULL);
    if (0 != status) {
      fprintf(stderr, "Failed to initialize mutex!\n");
      exit(-1);
    }
    status = sem_init(&work_tasks_, 0, 0);
    if (-1 == status) {
      fprintf(stderr, "Failed to initialize semaphore!\n");
      exit(-1);
    }
    status = sem_init(&done_tasks_, 0, 0);
    if (-1 == status) {
      fprintf(stderr, "Failed to initialize semaphore!\n");
      exit(-1);
    }
    threads_ = new pthread_t[num_threads_];
    for (int i = 0; i < num_threads_; i++) {
      status = pthread_create(&threads_[i], NULL, WorkerThreadEntry, this);
      if (0 != status) {
        fprintf(stderr, "Failed to create thread!\n");
        exit(-1);
      }
    }
  }
}

// Post exit request, wait for all threads to join, and cleanup.
ThreadPool::~ThreadPool() {
  if (num_threads_ > 1) {
    PostExitAndJoinAll();
    delete[] threads_;
    sem_destroy(&done_tasks_);
    sem_destroy(&work_tasks_);
    pthread_mutex_destroy(&mutex_);
  }
}

// Setup work parameters.  This function is called from the dispatch thread,
// when all worker threads are sleeping.
void ThreadPool::Setup(int counter, WorkFunction work, void *data) {
  counter_ = counter;
  user_work_function_ = work;
  user_data_ = data;
}

// Decrement and get the value of the mutex protected counter.  This function
// can be called from multiple threads at any given time.
int ThreadPool::DecCounter() {
  int v;
  pthread_mutex_lock(&mutex_);
  {
    v = --counter_;
  }
  pthread_mutex_unlock(&mutex_);
  return v;
}

// Set exit flag, post and join all the threads in the pool.  This function is
// called only from the dispatch thread, and only when all worker threads are
// sleeping.
void ThreadPool::PostExitAndJoinAll() {
  exiting_ = true;
  // Wake up all the sleeping worker threads.
  for (int i = 0; i < num_threads_; ++i)
    sem_post(&work_tasks_);
  void *retval;
  for (int i = 0; i < num_threads_; ++i)
    pthread_join(threads_[i], &retval);
}

// Main work loop - one for each worker thread.
void ThreadPool::WorkLoop() {
  while (true) {
    // Wait for work. If no work is availble, this thread will sleep.
    sem_wait(&work_tasks_);
    // Workers wake up from PostWork() issued from the dispatch thread.
    if (exiting_) break;
    // Grab a task index to work on from the counter.
    int task_index = DecCounter();
    if (task_index < 0) {
      // This indicates we're not sync'ing properly.
      fprintf(stderr, "Task index went negative!\n");
      exit(-1);
    }
    user_work_function_(task_index, user_data_);
    // Post to dispatch thread that a task completed.
    sem_post(&done_tasks_);
  }
}

// pthread entry point for a worker thread.
void* ThreadPool::WorkerThreadEntry(void *thiz) {
  static_cast<ThreadPool *>(thiz)->WorkLoop();
  return NULL;
}

// MultiThread() will dispatch a set of tasks across multiple worker threads.
// Note: This function will block until all work has completed.
void ThreadPool::MultiThread(int num_tasks, WorkFunction work, void *data) {
  // On entry, all worker threads are sleeping.
  Setup(num_tasks, work, data);

  // Wake up the worker threads & have them process the tasks.
  for (int i = 0; i < num_tasks; i++)
    sem_post(&work_tasks_);

  // Worker threads are now awake and busy.

  // This dispatch thread will now sleep-wait on the done_tasks_ semaphore,
  // waiting for the worker threads to finish all tasks.
  for (int i = 0; i < num_tasks; i++)
    sem_wait(&done_tasks_);

  // Make sure the counter is where we expect.
  int c = DecCounter();
  if (-1 != c) {
    fprintf(stderr, "We're not syncing correctly! (%d)\n", c);
    exit(-1);
  }
  // On exit, all tasks are done and all worker threads are sleeping again.
}

// SingleThread() will dispatch all work on this thread.
void ThreadPool::SingleThread(int num_tasks, WorkFunction work, void *data) {
  for (int i = 0; i < num_tasks; i++)
    work(i, data);
}

// Dispatch() will invoke the user supplied work function across
// one or more threads for each task.
// Note: This function will block until all work has completed.
void ThreadPool::Dispatch(int num_tasks, WorkFunction work, void *data) {
  if (num_threads_ > 1)
    MultiThread(num_tasks, work, data);
  else
    SingleThread(num_tasks, work, data);
}
#endif
