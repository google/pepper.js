// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Simple thread pool class

#ifndef EXAMPLES_DEMO_VORONOI_THREADPOOL_H_
#define EXAMPLES_DEMO_VORONOI_THREADPOOL_H_

#if !defined(DISABLE_THREADS)
#include <pthread.h>
#include <semaphore.h>
#endif

// typdef helper for work function
typedef void (*WorkFunction)(int task_index, void *data);

// ThreadPool is a class to manage num_threads and assign
// them num_tasks of work at a time. Each call
// to Dispatch(..) will block until all tasks complete.

class ThreadPool {
#if !defined(DISABLE_THREADS)
 public:
  void Dispatch(int num_tasks, WorkFunction work, void *data);
  explicit ThreadPool(int num_threads);
  ~ThreadPool();
 private:
  int DecCounter();
  void Setup(int counter, WorkFunction work, void *data);
  void MultiThread(int num_tasks, WorkFunction work, void *data);
  void SingleThread(int num_tasks, WorkFunction work, void *data);
  void WorkLoop();
  static void* WorkerThreadEntry(void *data);
  void PostExitAndJoinAll();
  pthread_t *threads_;
  int counter_;
  int num_threads_;
  bool exiting_;
  void *user_data_;
  WorkFunction user_work_function_;
  pthread_mutex_t mutex_;
  sem_t work_tasks_;
  sem_t done_tasks_;
#else
 public:
  void Dispatch(int num_tasks, WorkFunction work, void *data) {
    for(int i = 0; i < num_tasks; i++) work(i, data); }
  explicit ThreadPool(int num_threads) { ; }
  ~ThreadPool() { ; }
#endif  // DISABLE_THREADS
};
#endif  // EXAMPLES_DEMO_VORONOI_THREADPOOL_H_
