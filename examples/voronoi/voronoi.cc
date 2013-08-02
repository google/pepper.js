// Copyright 2013 The Native Client SDK Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <assert.h>
#include <math.h>
#include <ppapi/cpp/completion_callback.h>
#include <ppapi/cpp/graphics_2d.h>
#include <ppapi/cpp/image_data.h>
#include <ppapi/cpp/input_event.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/rect.h>
#include <ppapi/cpp/size.h>
#include <ppapi/cpp/var.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <string>

#include "threadpool.h"

// Global properties used to setup Voronoi demo.
namespace {
const int kMinRectSize = 4;
const int kStartRecurseSize = 32;
const float kHugeZ = 1.0e38f;
const float kPI = M_PI;
const float kTwoPI = kPI * 2.0f;
const int kFramesToBenchmark = 100;
const unsigned int kRandomStartSeed = 0xC0DE533D;
const int kMaxPointCount = 1024;
const int kStartPointCount = 256;

unsigned int g_rand_state = kRandomStartSeed;

// random number helper
inline unsigned char rand255() {
  return static_cast<unsigned char>(rand_r(&g_rand_state) & 255);
}

// random number helper
inline float frand() {
  return (static_cast<float>(rand_r(&g_rand_state)) /
      static_cast<float>(RAND_MAX));
}

// reset random seed
inline void rand_reset(unsigned int seed) {
  g_rand_state = seed;
}

inline double getseconds() {
  const double usec_to_sec = 0.000001;
  timeval tv;
  if (0 == gettimeofday(&tv, NULL))
    return tv.tv_sec + tv.tv_usec * usec_to_sec;
  return 0.0;
}

inline uint32_t MakeRGBA(uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
  return (((a) << 24) | ((b) << 16) | ((g) << 8) | (r));
}
}  // namespace

// Vec2, simple 2D vector
struct Vec2 {
  float x, y;
  Vec2() { ; }
  Vec2(float px, float py) {
    x = px;
    y = py;
  }
  void Set(float px, float py) {
    x = px;
    y = py;
  }
};

// The main object that runs Voronoi simulation.
class Voronoi : public pp::Instance {
 public:
  explicit Voronoi(PP_Instance instance);
  virtual ~Voronoi();

  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
    return true;
  }

  virtual void DidChangeView(const pp::Rect& position, const pp::Rect& clip);

  // Catch events.
  virtual bool HandleInputEvent(const pp::InputEvent& event);

  // Catch messages posted from Javascript.
  virtual void HandleMessage(const pp::Var& message);

 private:
  // Methods prefixed with 'w' are run on worker threads.
  int wCell(float x, float y);
  inline void wFillSpan(uint32_t *pixels, uint32_t color, int width);
  void wRenderTile(int x, int y, int w, int h);
  void wProcessTile(int x, int y, int w, int h);
  void wSubdivide(int x, int y, int w, int h);
  void wMakeRect(int region, int *x, int *y, int *w, int *h);
  bool wTestRect(int *m, int x, int y, int w, int h);
  void wFillRect(int x, int y, int w, int h, uint32_t color);
  void wRenderRect(int x0, int y0, int x1, int y1);
  void wRenderRegion(int region);
  static void wRenderRegionEntry(int region, void *thiz);

  // These methods are only called by the main thread.
  void Reset();
  void UpdateSim();
  void RenderDot(float x, float y, uint32_t color1, uint32_t color2);
  void SuperimposePositions();
  void Render();
  void Draw();
  void StartBenchmark();
  void EndBenchmark();

  // Runs a tick of the simulations, updating all buffers.  Flushes the
  // contents of |image_data_| to the 2D graphics context.
  void Update();

  // Create and initialize the 2D context used for drawing.
  void CreateContext(const pp::Size& size);
  // Destroy the 2D drawing context.
  void DestroyContext();
  // Push the pixels to the browser, then attempt to flush the 2D context.
  void FlushPixelBuffer();
  static void FlushCallback(void* data, int32_t result);


  pp::Graphics2D* graphics_2d_context_;
  pp::ImageData* image_data_;
  Vec2 positions_[kMaxPointCount];
  Vec2 screen_positions_[kMaxPointCount];
  Vec2 velocities_[kMaxPointCount];
  uint32_t colors_[kMaxPointCount];
  float ang_;
  int point_count_;
  int num_threads_;
  int num_regions_;
  bool draw_points_;
  bool draw_interiors_;
  ThreadPool *workers_;
  int width_;
  int height_;
  uint32_t stride_in_pixels_;
  uint32_t *pixel_buffer_;
  int benchmark_frame_counter_;
  bool benchmarking_;
  double benchmark_start_time_;
  double benchmark_end_time_;
};



void Voronoi::Reset() {
  rand_reset(kRandomStartSeed);
  ang_ = 0.0f;
  for (int i = 0; i < kMaxPointCount; i++) {
    // random initial start position
    const float x = frand();
    const float y = frand();
    positions_[i].Set(x, y);
    // random directional velocity ( -1..1, -1..1 )
    const float speed = 0.0005f;
    const float u = (frand() * 2.0f - 1.0f) * speed;
    const float v = (frand() * 2.0f - 1.0f) * speed;
    velocities_[i].Set(u, v);
    // 'unique' color (well... unique enough for our purposes)
    colors_[i] = MakeRGBA(rand255(), rand255(), rand255(), 255);
  }
}

Voronoi::Voronoi(PP_Instance instance) : pp::Instance(instance),
                                         graphics_2d_context_(NULL),
                                         image_data_(NULL) {
  draw_points_ = true;
  draw_interiors_ = true;
  width_ = 0;
  height_ = 0;
  stride_in_pixels_ = 0;
  pixel_buffer_ = NULL;
  benchmark_frame_counter_ = 0;
  benchmarking_ = false;

  point_count_ = kStartPointCount;
  Reset();

  // By default, do single threaded rendering.
  num_regions_ = 256;
  num_threads_ = 1;
  workers_ = new ThreadPool(num_threads_);

  // Request PPAPI input events for mouse & keyboard.
  RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE);
  RequestInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD);
}

Voronoi::~Voronoi() {
  delete workers_;
  DestroyContext();
}

// This is the core of the Voronoi calculation.  At a given point on the
// screen, iterate through all voronoi positions and render them as 3D cones.
// We're looking for the voronoi cell that generates the closest z value.
// (not really cones - since it is all realative, we avoid doing the
// expensive sqrt and test against z*z instead)
// If multithreading, this function is only called by the worker threads.
int Voronoi::wCell(float x, float y) {
  int n = 0;
  float zz = kHugeZ;
  Vec2 *pos = screen_positions_;
  for (int i = 0; i < point_count_; ++i) {
    // measured 5.18 cycles per iteration on a core2
    float dx = x - pos[i].x;
    float dy = y - pos[i].y;
    float dd = (dx * dx + dy * dy);
    if (dd < zz) {
      zz = dd;
      n = i;
    }
  }
  // Return the closest cell to point x,y.
  return n;
}

// Given a region r, derive a non-overlapping rectangle for a thread to
// work on.
// If multithreading, this function is only called by the worker threads.
void Voronoi::wMakeRect(int r, int *x, int *y, int *w, int *h) {
  const int parts = 16;
  assert(parts * parts == num_regions_);
  *w = width_ / parts;
  *h = height_ / parts;
  *x = *w * (r % parts);
  *y = *h * ((r / parts) % parts);
}

// Test 4 corners of a rectangle to see if they all belong to the same
// voronoi cell.  Each test is expensive so bail asap.  Returns true
// if all 4 corners match.
// If multithreading, this function is only called by the worker threads.
bool Voronoi::wTestRect(int *m, int x, int y, int w, int h) {
  // each test is expensive, so exit ASAP
  const int m0 = wCell(x, y);
  const int m1 = wCell(x + w - 1, y);
  if (m0 != m1) return false;
  const int m2 = wCell(x, y + h - 1);
  if (m0 != m2) return false;
  const int m3 = wCell(x + w - 1, y + h - 1);
  if (m0 != m3) return false;
  // all 4 corners belong to the same cell
  *m = m0;
  return true;
}

// Quickly fill a span of pixels with a solid color.  Assumes
// span width is divisible by 4.
// If multithreading, this function is only called by the worker threads.
inline void Voronoi::wFillSpan(uint32_t *pixels, uint32_t color, int width) {
  if (!draw_interiors_) {
    const uint32_t gray = MakeRGBA(128, 128, 128, 255);
    color = gray;
  }
  for (int i = 0; i < width; i += 4) {
    *pixels++ = color;
    *pixels++ = color;
    *pixels++ = color;
    *pixels++ = color;
  }
}

// Quickly fill a rectangle with a solid color.  Assumes
// the width w parameter is evenly divisible by 4.
// If multithreading, this function is only called by the worker threads.
void Voronoi::wFillRect(int x, int y, int w, int h, uint32_t color) {
  const uint32_t pitch = width_;
  uint32_t *pixels = pixel_buffer_ + y * pitch + x;
  for (int j = 0; j < h; j++) {
    wFillSpan(pixels, color, w);
    pixels += pitch;
  }
}

// When recursive subdivision reaches a certain minimum without finding a
// rectangle that has four matching corners belonging to the same voronoi
// cell, this function will break the retangular 'tile' into smaller scanlines
//  and look for opportunities to quick fill at the scanline level.  If the
// scanline can't be quick filled, it will slow down even further and compute
// voronoi membership per pixel.
void Voronoi::wRenderTile(int x, int y, int w, int h) {
  // rip through a tile
  uint32_t *pixels = pixel_buffer_ + y * stride_in_pixels_ + x;
  for (int j = 0; j < h; j++) {
    // get start and end cell values
    int ms = wCell(x + 0, y + j);
    int me = wCell(x + w - 1, y + j);
    // if the end points are the same, quick fill the span
    if (ms == me) {
      wFillSpan(pixels, colors_[ms], w);
    } else {
      // else compute each pixel in the span... this is the slow part!
      uint32_t *p = pixels;
      *p++ = colors_[ms];
      for (int i = 1; i < (w - 1); i++) {
        int m = wCell(x + i, y + j);
        *p++ = colors_[m];
      }
      *p++ = colors_[me];
    }
    pixels += stride_in_pixels_;
  }
}

// Take a rectangular region and do one of -
//   If all four corners below to the same voronoi cell, stop recursion and
// quick fill the rectangle.
//   If the minimum rectangle size has been reached, break out of recursion
// and process the rectangle.  This small rectangle is called a tile.
//   Otherwise, keep recursively subdividing the rectangle into 4 equally
// sized smaller rectangles.
// Note: at the moment, these will always be squares, not rectangles.
// If multithreading, this function is only called by the worker threads.
void Voronoi::wSubdivide(int x, int y, int w, int h) {
  int m;
  // if all 4 corners are equal, quick fill interior
  if (wTestRect(&m, x, y, w, h)) {
    wFillRect(x, y, w, h, colors_[m]);
  } else {
    // did we reach the minimum rectangle size?
    if ((w <= kMinRectSize) || (h <= kMinRectSize)) {
      wRenderTile(x, y, w, h);
    } else {
      // else recurse into smaller rectangles
      const int half_w = w / 2;
      const int half_h = h / 2;
      wSubdivide(x, y, half_w, half_h);
      wSubdivide(x + half_w, y, half_w, half_h);
      wSubdivide(x, y + half_h, half_w, half_h);
      wSubdivide(x + half_w, y + half_h, half_w, half_h);
    }
  }
}

// This function cuts up the rectangle into power of 2 sized squares.  It
// assumes the input rectangle w & h are evenly divisible by
// kStartRecurseSize.
// If multithreading, this function is only called by the worker threads.
void Voronoi::wRenderRect(int x, int y, int w, int h) {
  for (int iy = y; iy < (y + h); iy += kStartRecurseSize) {
    for (int ix = x; ix < (x + w); ix += kStartRecurseSize) {
      wSubdivide(ix, iy, kStartRecurseSize, kStartRecurseSize);
    }
  }
}

// If multithreading, this function is only called by the worker threads.
void Voronoi::wRenderRegion(int region) {
  // convert region # into x0, y0, x1, y1 rectangle
  int x, y, w, h;
  wMakeRect(region, &x, &y, &w, &h);
  // render this rectangle
  wRenderRect(x, y, w, h);
}

// Entry point for worker thread.  Can't pass a member function around, so we
// have to do this little round-about.
void Voronoi::wRenderRegionEntry(int region, void *thiz) {
  static_cast<Voronoi*>(thiz)->wRenderRegion(region);
}

// Function Voronoi::UpdateSim()
// Run a simple sim to move the voronoi positions.  This update loop
// is run once per frame.  Called from the main thread only, and only
// when the worker threads are idle.
void Voronoi::UpdateSim() {
  ang_ += 0.002f;
  if (ang_ > kTwoPI) {
    ang_ = ang_ - kTwoPI;
  }
  float z = cosf(ang_) * 3.0f;
  // push the points around on the screen for animation
  for (int j = 0; j < kMaxPointCount; j++) {
    positions_[j].x += (velocities_[j].x) * z;
    positions_[j].y += (velocities_[j].y) * z;
    screen_positions_[j].x = positions_[j].x * width_;
    screen_positions_[j].y = positions_[j].y * height_;
  }
}

// Renders a small diamond shaped dot at x, y clipped against the window
void Voronoi::RenderDot(float x, float y, uint32_t color1, uint32_t color2) {
  const int ix = static_cast<int>(x);
  const int iy = static_cast<int>(y);
  // clip it against window
  if (ix < 1) return;
  if (ix >= (width_ - 1)) return;
  if (iy < 1) return;
  if (iy >= (height_ - 1)) return;
  uint32_t *pixel = pixel_buffer_ + iy * stride_in_pixels_ + ix;
  // render dot as a small diamond
  *pixel = color1;
  *(pixel - 1) = color2;
  *(pixel + 1) = color2;
  *(pixel - stride_in_pixels_) = color2;
  *(pixel + stride_in_pixels_) = color2;
}

// Superimposes dots on the positions.
void Voronoi::SuperimposePositions() {
  const uint32_t white = MakeRGBA(255, 255, 255, 255);
  const uint32_t gray = MakeRGBA(192, 192, 192, 255);
  for (int i = 0; i < point_count_; i++) {
    RenderDot(
        screen_positions_[i].x, screen_positions_[i].y, white, gray);
  }
}

// Renders the Voronoi diagram, dispatching the work to multiple threads.
// Note: This Dispatch() is from the main PPAPI thread, so care must be taken
// not to attempt PPAPI calls from the worker threads, since Dispatch() will
// block here until all work is complete.  The worker threads are compute only
// and do not make any PPAPI calls.
void Voronoi::Render() {
  workers_->Dispatch(num_regions_, wRenderRegionEntry, this);
  if (draw_points_)
    SuperimposePositions();
}

void Voronoi::DidChangeView(const pp::Rect& position, const pp::Rect& clip) {
  if (position.size().width() == width_ &&
      position.size().height() == height_)
    return;  // Size didn't change, no need to update anything.

  // Create a new device context with the new size.
  DestroyContext();
  CreateContext(position.size());
  Update();
}

void Voronoi::StartBenchmark() {
  Reset();
  printf("Benchmark started...\n");
  benchmark_frame_counter_ = kFramesToBenchmark;
  benchmarking_ = true;
  benchmark_start_time_ = getseconds();
}

void Voronoi::EndBenchmark() {
  benchmark_end_time_ = getseconds();
  printf("Benchmark ended... time: %2.5f\n",
      benchmark_end_time_ - benchmark_start_time_);
  benchmarking_ = false;
  benchmark_frame_counter_ = 0;
  pp::Var result(benchmark_end_time_ - benchmark_start_time_);
  PostMessage(result);
}

// Handle input events from the user.
bool Voronoi::HandleInputEvent(const pp::InputEvent& event) {
  switch (event.GetType()) {
    case PP_INPUTEVENT_TYPE_KEYDOWN: {
      pp::KeyboardInputEvent key = pp::KeyboardInputEvent(event);
      uint32_t key_code = key.GetKeyCode();
      if (key_code == 84)  // 't' key
        if (!benchmarking_)
          StartBenchmark();
      break;
    }
    default:
      return false;
  }
  return true;
}

// Handle messages sent from Javascript.
void Voronoi::HandleMessage(const pp::Var& message) {
  if (message.is_string()) {
    std::string message_string = message.AsString();
    if (message_string == "run benchmark" && !benchmarking_)
      StartBenchmark();
    else if (message_string == "with points")
      draw_points_ = true;
    else if (message_string == "without points")
      draw_points_ = false;
    else if (message_string == "with interiors")
      draw_interiors_ = true;
    else if (message_string == "without interiors")
      draw_interiors_ = false;
    else if (strstr(message_string.c_str(), "points:")) {
      int num_points = atoi(strstr(message_string.c_str(), " "));
      point_count_ = num_points < kMaxPointCount ? num_points : kMaxPointCount;
    } else if (strstr(message_string.c_str(), "threads:")) {
      int thread_count = atoi(strstr(message_string.c_str(), " "));
      delete workers_;
      workers_ = new ThreadPool(thread_count);
    }
  }
}

void Voronoi::FlushCallback(void* thiz, int32_t result) {
  static_cast<Voronoi*>(thiz)->Update();
}

// Update the 2d region and flush to make it visible on the page.
void Voronoi::FlushPixelBuffer() {
  graphics_2d_context_->PaintImageData(*image_data_, pp::Point(0, 0));
  graphics_2d_context_->Flush(pp::CompletionCallback(&FlushCallback, this));
}

void Voronoi::Update() {
  // Don't call FlushPixelBuffer() when benchmarking - vsync is enabled by
  // default, and will throttle the benchmark results.
  do {
    UpdateSim();
    Render();
    if (!benchmarking_) break;
    --benchmark_frame_counter_;
  } while (benchmark_frame_counter_ > 0);
  if (benchmarking_)
    EndBenchmark();
  FlushPixelBuffer();
}

void Voronoi::CreateContext(const pp::Size& size) {
  const size_t bytes = size.width() * size.height();
  graphics_2d_context_ = new pp::Graphics2D(this, size, false);
  if (!BindGraphics(*graphics_2d_context_))
    printf("Couldn't bind the device context\n");
  image_data_ = new pp::ImageData(this,
                                  PP_IMAGEDATAFORMAT_RGBA_PREMUL,
                                  size,
                                  false);
  width_ = image_data_->size().width();
  height_ = image_data_->size().height();
  stride_in_pixels_ = static_cast<uint32_t>(image_data_->stride() / 4);
  pixel_buffer_ = static_cast<uint32_t*>(image_data_->data());
}

void Voronoi::DestroyContext() {
  delete graphics_2d_context_;
  delete image_data_;
  graphics_2d_context_ = NULL;
  image_data_ = NULL;
  width_ = 0;
  height_ = 0;
  stride_in_pixels_ = 0;
  pixel_buffer_ = NULL;
}

// The Module class.  The browser calls the CreateInstance() method to create
// an instance of you NaCl module on the web page.  The browser creates a new
// instance for each <embed> tag with type="application/x-nacl".
class VoronoiModule : public pp::Module {
 public:
  VoronoiModule() : pp::Module() {}
  virtual ~VoronoiModule() {}

  // Create and return a Voronoi instance.
  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new Voronoi(instance);
  }
};

// Factory function called by the browser when the module is first loaded.
// The browser keeps a singleton of this module.  It calls the
// CreateInstance() method on the object you return to make instances.  There
// is one instance per <embed> tag on the page.  This is the main binding
// point for your NaCl module with the browser.
namespace pp {
Module* CreateModule() {
  return new VoronoiModule();
}
}  // namespace pp

