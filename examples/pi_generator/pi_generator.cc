// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <cmath>
#include <cstring>
#include <string>
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/var.h"

#include "pi_generator.h"

namespace {
const int kPthreadMutexSuccess = 0;
const char* const kPaintMethodId = "paint";
const double kInvalidPiValue = -1.0;
const int kMaxPointCount = 1000000000;  // The total number of points to draw.
const uint32_t kOpaqueColorMask = 0xff000000;  // Opaque pixels.
const uint32_t kRedMask = 0xff0000;
const uint32_t kBlueMask = 0xff;
const uint32_t kRedShift = 16;
const uint32_t kBlueShift = 0;

// This is called by the browser when the 2D context has been flushed to the
// browser window.
void FlushCallback(void* data, int32_t result) {
  static_cast<pi_generator::PiGenerator*>(data)->set_flush_pending(false);
}

}  // namespace

namespace pi_generator {

// A small helper RAII class that implementes a scoped pthread_mutex lock.
class ScopedMutexLock {
 public:
  explicit ScopedMutexLock(pthread_mutex_t* mutex) : mutex_(mutex) {
    if (pthread_mutex_lock(mutex_) != kPthreadMutexSuccess) {
      mutex_ = NULL;
    }
  }
  ~ScopedMutexLock() {
    if (mutex_)
      pthread_mutex_unlock(mutex_);
  }
  bool is_valid() const {
    return mutex_ != NULL;
  }
 private:
  pthread_mutex_t* mutex_;  // Weak reference.
};

// A small helper RAII class used to acquire and release the pixel lock.
class ScopedPixelLock {
 public:
  explicit ScopedPixelLock(PiGenerator* image_owner)
      : image_owner_(image_owner), pixels_(image_owner->LockPixels()) {}

  ~ScopedPixelLock() {
    pixels_ = NULL;
    image_owner_->UnlockPixels();
  }

  uint32_t* pixels() const {
    return pixels_;
  }
 private:
  PiGenerator* image_owner_;  // Weak reference.
  uint32_t* pixels_;  // Weak reference.

  ScopedPixelLock();  // Not implemented, do not use.
};

PiGenerator::PiGenerator(PP_Instance instance)
    : pp::Instance(instance),
      graphics_2d_context_(NULL),
      pixel_buffer_(NULL),
      flush_pending_(false),
      quit_(false),
      thread_create_result_(0),
      pi_(0.0),
      num_inside(0),
      num_samples(0) {
  pthread_mutex_init(&pixel_buffer_mutex_, NULL);
}

PiGenerator::~PiGenerator() {
  quit_ = true;
  if (thread_create_result_ == 0) {
    pthread_join(compute_pi_thread_, NULL);
  }
  DestroyContext();
  // The ComputePi() thread should be gone by now, so there is no need to
  // acquire the mutex for |pixel_buffer_|.
  delete pixel_buffer_;
  pthread_mutex_destroy(&pixel_buffer_mutex_);
}

void PiGenerator::DidChangeView(const pp::View& view) {
  pp::Rect position = view.GetRect();
  pp::Size desired;
  desired.set_width(position.size().width() * view.GetDeviceScale());
  desired.set_height(position.size().height() * view.GetDeviceScale());
  if (pixel_buffer_ && desired == pixel_buffer_->size())
    return;  // Size didn't change, no need to update anything.

  // Create a new device context with the new size.
  DestroyContext();
  CreateContext(desired);
  graphics_2d_context_->SetScale(1.0/view.GetDeviceScale());
  // Delete the old pixel buffer and create a new one.
  ScopedMutexLock scoped_mutex(&pixel_buffer_mutex_);
  delete pixel_buffer_;
  pixel_buffer_ = NULL;
  if (graphics_2d_context_ != NULL) {
    pixel_buffer_ = new pp::ImageData(this,
                                     PP_IMAGEDATAFORMAT_BGRA_PREMUL,
                                      graphics_2d_context_->size(),
                                      false);
  }
}


void Pump(void* param, int result) {
  PiGenerator* pi_generator = static_cast<PiGenerator*>(param);
  pi_generator->Work();
  pp::Module::Get()->core()->CallOnMainThread(0, pp::CompletionCallback(&Pump, param));
}

bool PiGenerator::Init(uint32_t argc, const char* argn[], const char* argv[]) {
  prev_time = pp::Module::Get()->core()->GetTime();
  timed_samples = 0;
  pp::Module::Get()->core()->CallOnMainThread(0, pp::CompletionCallback(&Pump, this));
  return true;

  thread_create_result_ = pthread_create(&compute_pi_thread_, NULL, ComputePi, this);
  return thread_create_result_ == 0;
}

uint32_t* PiGenerator::LockPixels() {
  void* pixels = NULL;
  // Do not use a ScopedMutexLock here, since the lock needs to be held until
  // the matching UnlockPixels() call.
  if (pthread_mutex_lock(&pixel_buffer_mutex_) == kPthreadMutexSuccess) {
    if (pixel_buffer_ != NULL && !pixel_buffer_->is_null()) {
      pixels = pixel_buffer_->data();
    }
  }
  return reinterpret_cast<uint32_t*>(pixels);
}

void PiGenerator::HandleMessage(const pp::Var& var_message) {
  if (!var_message.is_string()) {
    PostMessage(pp::Var(kInvalidPiValue));
  }
  std::string message = var_message.AsString();
  if (message == kPaintMethodId) {
    Paint();
  } else {
    PostMessage(pp::Var(kInvalidPiValue));
  }
}

void PiGenerator::UnlockPixels() const {
  pthread_mutex_unlock(&pixel_buffer_mutex_);
}

void PiGenerator::Paint() {
  ScopedMutexLock scoped_mutex(&pixel_buffer_mutex_);
  if (!scoped_mutex.is_valid()) {
    return;
  }
  FlushPixelBuffer();
  // Post the current estimate of Pi back to the browser.
  pp::Var pi_estimate(pi());
  // Paint() is called on the main thread, so no need for CallOnMainThread()
  // here.  It's OK to just post the message.
  PostMessage(pi_estimate);
}

void PiGenerator::CreateContext(const pp::Size& size) {
  ScopedMutexLock scoped_mutex(&pixel_buffer_mutex_);
  if (!scoped_mutex.is_valid()) {
    return;
  }
  if (IsContextValid())
    return;
  graphics_2d_context_ = new pp::Graphics2D(this, size, false);
  if (!BindGraphics(*graphics_2d_context_)) {
    printf("Couldn't bind the device context\n");
  }
}

void PiGenerator::DestroyContext() {
  ScopedMutexLock scoped_mutex(&pixel_buffer_mutex_);
  if (!scoped_mutex.is_valid()) {
    return;
  }
  if (!IsContextValid())
    return;
  delete graphics_2d_context_;
  graphics_2d_context_ = NULL;
}

void PiGenerator::FlushPixelBuffer() {
  if (!IsContextValid())
    return;
  // Note that the pixel lock is held while the buffer is copied into the
  // device context and then flushed.
  graphics_2d_context_->PaintImageData(*pixel_buffer_, pp::Point());
  if (flush_pending())
    return;
  set_flush_pending(true);
  graphics_2d_context_->Flush(pp::CompletionCallback(&FlushCallback, this));
}

void PiGenerator::UpdateEstimate(bool inside) {
  num_samples += 1;
  timed_samples += 1;
  if (inside)
    num_inside += 1;
  pi_ = 4.0 * num_inside / num_samples; 
}

void PiGenerator::ComputeStep(uint32_t* pixel_bits) {
  double x = static_cast<double>(rand()) / RAND_MAX;
  double y = static_cast<double>(rand()) / RAND_MAX;
  double distance = sqrt(x * x + y * y);
  bool inside = distance < 1.0;
  UpdateEstimate(inside);
  DrawPoint(x, y, inside, pixel_bits);
}

void PiGenerator::DrawPoint(double x, double y, bool inside, uint32_t* pixel_bits) {
  if (pixel_bits == NULL) {
      // Note that if the pixel buffer never gets initialized, this won't ever
      // paint anything.  Which is probably the right thing to do.  Also, this
      // clause means that the image will not get the very first few Pi dots,
      // since it's possible that this thread starts before the pixel buffer is
      // initialized.
      return;
  }
  int px = x * width();
  int py = (1.0 - y) * height();
  uint32_t offset = width() * py + px;
  uint32_t color = pixel_bits[offset];
  if (inside) {
    // Set color to blue.
    color += 4 << kBlueShift;
    color &= kBlueMask;
  } else {
    // Set color to red.
    color += 4 << kRedShift;
    color &= kRedMask;
  }
  pixel_bits[offset] = color | kOpaqueColorMask;
}

void PiGenerator::Work() {
  ScopedPixelLock scoped_pixel_lock(this);
  uint32_t* pixel_bits = scoped_pixel_lock.pixels();
  for (int i=0; i<10000; i++) {
    ComputeStep(pixel_bits);
  }
  double current = pp::Module::Get()->core()->GetTime();
  double delta = current - prev_time;
  if (delta >= 4) {
    prev_time = current;
    char buf[1024];
    sprintf(buf, "%d", (int)(timed_samples / delta));
    std::string message = "Samples/sec: " + std::string(buf);
    LogToConsole(PP_LOGLEVEL_LOG, pp::Var(message));
    timed_samples = 0;
  }
}

void* PiGenerator::ComputePi(void* param) {
  int count = 0;  // The number of points put inside the inscribed quadrant.
  unsigned int seed = 1;
  srand(seed);

  PiGenerator* pi_generator = static_cast<PiGenerator*>(param);
  for (int i = 0; i < kMaxPointCount && !pi_generator->quit(); ++i) {
    ScopedPixelLock scoped_pixel_lock(pi_generator);
    uint32_t* pixel_bits = scoped_pixel_lock.pixels();
    pi_generator->ComputeStep(pixel_bits);
  }
  return 0;
}

}  // namespace pi_generator
