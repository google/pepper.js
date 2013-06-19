// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <math.h>
#include <ppapi/c/pp_point.h>
#include <ppapi/cpp/completion_callback.h>
#include <ppapi/cpp/graphics_2d.h>
#include <ppapi/cpp/image_data.h>
#include <ppapi/cpp/input_event.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/rect.h>
#include <ppapi/cpp/size.h>
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_array_buffer.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

#include "sdk_util/thread_pool.h"

// Global properties used to setup Earth demo.
namespace {
const float kHugeZ = 1.0e38f;
const float kPI = M_PI;
const float kTwoPI = kPI * 2.0f;
const float kOneOverPI = 1.0f / kPI;
const float kOneOver2PI = 1.0f / kTwoPI;
const float kOneOver255 = 1.0f / 255.0f;
const int kArcCosineTableSize = 4096;
const int kFramesToBenchmark = 100;
const float kZoomMin = 1.0f;
const float kZoomMax = 50.0f;
const float kWheelSpeed = 2.0f;
const float kLightMin = 0.0f;
const float kLightMax = 2.0f;
const int kFrameTimeBufferSize = 512;

// Timer helper for benchmarking.  Returns seconds elapsed since program start.
timeval start_tv;
int start_tv_retv = gettimeofday(&start_tv, NULL);

inline double getseconds() {
  const double usec_to_sec = 0.000001;
  timeval tv;
  if ((0 == start_tv_retv) && (0 == gettimeofday(&tv, NULL)))
    return (tv.tv_sec - start_tv.tv_sec) + tv.tv_usec * usec_to_sec;
  return 0.0;
}

// RGBA helper functions.
inline float ExtractR(uint32_t c) {
  return static_cast<float>(c & 0xFF) * kOneOver255;
}

inline float ExtractG(uint32_t c) {
  return static_cast<float>((c & 0xFF00) >> 8) * kOneOver255;
}

inline float ExtractB(uint32_t c) {
  return static_cast<float>((c & 0xFF0000) >> 16) * kOneOver255;
}

inline uint32_t MakeRGBA(uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
  return (((a) << 24) | ((r) << 16) | ((g) << 8) | (b));
}

// simple container for earth texture
struct Texture {
  int width, height;
  uint32_t* pixels;
  explicit Texture(int w, int h) : width(w), height(h) {
    pixels = new uint32_t[w * h];
    for (int i = 0; i < w * h; i++) pixels[i] = 0;
  }
  explicit Texture(int w, int h, uint32_t* p) : width(w), height(h) {
    pixels = new uint32_t[w * h];
    for (int i = 0; i < w * h; i++) pixels[i] = p[i];
  }
  ~Texture() { delete[] pixels; }
};



struct ArcCosine {
  // slightly larger table so we can interpolate beyond table size
  float table[kArcCosineTableSize + 2];
  float TableLerp(float x);
  ArcCosine();
  ~ArcCosine() { ; }
};

ArcCosine::ArcCosine() {
  // build a slightly larger table to allow for numeric imprecision
  for (int i = 0; i < (kArcCosineTableSize + 2); ++i) {
    float f = static_cast<float>(i) / kArcCosineTableSize;
    f = f * 2.0f - 1.0f;
    table[i] = acos(f);
  }
}

// looks up acos(f) using a table and lerping between entries
// (it is expected that input f is between -1 and 1)
float ArcCosine::TableLerp(float f) {
  float x = (f + 1.0f) * 0.5f;
  x = x * kArcCosineTableSize;
  int ix = static_cast<int>(x);
  float fx = static_cast<float>(ix);
  float dx = x - fx;
  float af = table[ix];
  float af2 = table[ix + 1];
  float a = af + (af2 - af) * dx;
  return a;
}

// Helper functions for quick but approximate sqrt.
union Convert {
  float f;
  int i;
  Convert(int x) { i = x; }
  Convert(float x) { f = x; }
  const int asInt() { return i; }
  const float asFloat() { return f; }
};

inline const int AsInteger(const float f) {
  Convert u(f);
  return u.asInt();
}

inline const float AsFloat(const int i) {
  Convert u(i);
  return u.asFloat();
}

const long int kOneAsInteger = AsInteger(1.0f);
const float kScaleUp = float(0x00800000);
const float kScaleDown = 1.0f / kScaleUp;

inline float inline_quick_sqrt(float x) {
  int i;
  i = (AsInteger(x) >> 1) + (kOneAsInteger >> 1);
  return AsFloat(i);
}

inline float inline_sqrt(float x) {
  float y;
  y = inline_quick_sqrt(x);
  y = (y * y + x) / (2.0f * y);
  y = (y * y + x) / (2.0f * y);
  return y;
}

// takes a -0..1+ color, clamps it to 0..1 and maps it to 0..255 integer
inline unsigned int Clamp255(float x) {
  if (x < 0.0f) {
    x = 0.0f;
  } else if (x > 1.0f) {
    x = 1.0f;
  }
  return (unsigned int)(x * 255.0f);
}
}  // namespace


// The main object that runs the Earth demo.
class Planet : public pp::Instance {
 public:
  explicit Planet(PP_Instance instance);
  virtual ~Planet();

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
  uint32_t* wGetAddr(int x, int y);
  void wRenderPixelSpan(int x0, int x1, int y);
  void wMakeRect(int r, int *x, int *y, int *w, int *h);
  void wRenderRect(int x0, int y0, int x1, int y1);
  void wRenderRegion(int region);
  static void wRenderRegionEntry(int region, void *thiz);

  // These methods are only called by the main thread.
  void CacheCalcs();
  void SetPlanetXYZR(float x, float y, float z, float r);
  void SetPlanetPole(float x, float y, float z);
  void SetPlanetEquator(float x, float y, float z);
  void SetPlanetSpin(float x, float y);
  void SetEyeXYZ(float x, float y, float z);
  void SetLightXYZ(float x, float y, float z);
  void SetAmbientRGB(float r, float g, float b);
  void SetDiffuseRGB(float r, float g, float b);
  void SetZoom(float zoom, bool send_message);
  void SetLight(float zoom, bool send_message);

  void Reset();
  void UpdateSim();
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

  // User Interface settings.  These settings are controlled via html
  // controls or via user input.
  float ui_light_;
  float ui_zoom_;
  float ui_spin_x_;
  float ui_spin_y_;

  // Various settings for position & orientation of planet.  Do not change
  // these variables, instead use SetPlanet*() functions.
  float planet_radius_;
  float planet_spin_x_;
  float planet_spin_y_;
  float planet_x_, planet_y_, planet_z_;
  float planet_pole_x_, planet_pole_y_, planet_pole_z_;
  float planet_equator_x_, planet_equator_y_, planet_equator_z_;

  // Observer's eye.  Do not change these variables, instead use SetEyeXYZ().
  float eye_x_, eye_y_, eye_z_;

  // Light position, ambient and diffuse settings.  Do not change these
  // variables, instead use SetLightXYZ(), SetAmbientRGB() and SetDiffuseRGB().
  float light_x_, light_y_, light_z_;
  float diffuse_r_, diffuse_g_, diffuse_b_;
  float ambient_r_, ambient_g_, ambient_b_;

  // Cached calculations.  Do not change these variables - they are updated by
  // CacheCalcs() function.
  float planet_xyz_;
  float planet_pole_x_equator_x_;
  float planet_pole_x_equator_y_;
  float planet_pole_x_equator_z_;
  float planet_radius2_;
  float planet_one_over_radius_;
  float eye_xyz_;

  // Source texture (earth map).
  Texture *base_tex_;
  Texture *night_tex_;
  int width_for_tex_;
  int height_for_tex_;

  // Quick ArcCos helper.
  ArcCosine acos_;

  // Misc.
  pp::Graphics2D* graphics_2d_context_;
  pp::ImageData* image_data_;
  int num_threads_;
  int num_regions_;
  ThreadPool* workers_;
  int width_;
  int height_;
  uint32_t stride_in_pixels_;
  uint32_t* pixel_buffer_;
  int benchmark_frame_counter_;
  bool benchmarking_;
  double benchmark_start_time_;
  double benchmark_end_time_;
};



void Planet::Reset() {
  // Reset has to first fill in all variables with valid floats, so
  // CacheCalcs() doesn't potentially propagate NaNs when calling Set*()
  // functions further below.
  planet_radius_ = 1.0f;
  planet_spin_x_ = 0.0f;
  planet_spin_y_ = 0.0f;
  planet_x_ = 0.0f;
  planet_y_ = 0.0f;
  planet_z_ = 0.0f;
  planet_pole_x_ = 0.0f;
  planet_pole_y_ = 0.0f;
  planet_pole_z_ = 0.0f;
  planet_equator_x_ = 0.0f;
  planet_equator_y_ = 0.0f;
  planet_equator_z_ = 0.0f;
  eye_x_ = 0.0f;
  eye_y_ = 0.0f;
  eye_z_ = 0.0f;
  light_x_ = 0.0f;
  light_y_ = 0.0f;
  light_z_ = 0.0f;
  diffuse_r_ = 0.0f;
  diffuse_g_ = 0.0f;
  diffuse_b_ = 0.0f;
  ambient_r_ = 0.0f;
  ambient_g_ = 0.0f;
  ambient_b_ = 0.0f;
  planet_xyz_ = 0.0f;
  planet_pole_x_equator_x_ = 0.0f;
  planet_pole_x_equator_y_ = 0.0f;
  planet_pole_x_equator_z_ = 0.0f;
  planet_radius2_ = 0.0f;
  planet_one_over_radius_ = 0.0f;
  eye_xyz_ = 0.0f;
  ui_zoom_ = 14.0f;
  ui_light_ = 1.0f;
  ui_spin_x_ = 0.01f;
  ui_spin_y_ = 0.0f;

  // Set up reasonable default values.
  SetPlanetXYZR(0.0f, 0.0f, 48.0f, 4.0f);
  SetEyeXYZ(0.0f, 0.0f, -ui_zoom_);
  SetLightXYZ(-60.0f, -30.0f, 0.0f);
  SetAmbientRGB(0.05f, 0.05f, 0.05f);
  SetDiffuseRGB(0.8f, 0.8f, 0.8f);
  SetPlanetPole(0.0f, 1.0f, 0.0f);
  SetPlanetEquator(1.0f, 0.0f, 0.0f);
  SetPlanetSpin(kPI / 2.0f, kPI / 2.0f);
  SetZoom(ui_zoom_, true);
  SetLight(ui_light_, true);
}

Planet::Planet(PP_Instance instance) : pp::Instance(instance),
                                       graphics_2d_context_(NULL),
                                       image_data_(NULL),
                                       num_regions_(256) {
  width_ = 0;
  height_ = 0;
  stride_in_pixels_ = 0;
  pixel_buffer_ = NULL;
  benchmark_frame_counter_ = 0;
  benchmarking_ = false;
  base_tex_ = NULL;
  night_tex_ = NULL;

  Reset();

  // By default, render from the dispatch thread.
  num_threads_ = 0;
  workers_ = new ThreadPool(num_threads_);

  // Request PPAPI input events for mouse & keyboard.
  RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE);
  RequestInputEvents(PP_INPUTEVENT_CLASS_WHEEL);
  RequestInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD);
}

Planet::~Planet() {
  delete workers_;
  DestroyContext();
}

// Given a region r, derive a rectangle.
// This rectangle shouldn't overlap with work being done by other workers.
// If multithreading, this function is only called by the worker threads.
void Planet::wMakeRect(int r, int *x, int *y, int *w, int *h) {
  int dy = height_ / num_regions_;
  *x = 0;
  *w = width_;
  *y = r * dy;
  *h = dy;
}


inline uint32_t* Planet::wGetAddr(int x, int y) {
  assert(pixel_buffer_);
  return (pixel_buffer_ + y * stride_in_pixels_) + x;
}

// This is the meat of the ray tracer.  Given a pixel span (x0, x1) on
// scanline y, shoot rays into the scene and render what they hit.  Use
// scanline coherence to do a few optimizations
void Planet::wRenderPixelSpan(int x0, int x1, int y) {
  if (!base_tex_ || !night_tex_)
    return;
  const int kColorBlack = MakeRGBA(0, 0, 0, 0xFF);
  float y0 = eye_y_;
  float z0 = eye_z_;
  float y1 = (static_cast<float>(y) / height_) * 2.0f - 1.0f;
  float z1 = 0.0f;
  float dy = (y1 - y0);
  float dz = (z1 - z0);
  float dy_dy_dz_dz = dy * dy + dz * dz;
  float two_dy_y0_y_two_dz_z0_z = 2.0f * dy * (y0 - planet_y_) +
                                  2.0f * dz * (z0 - planet_z_);
  float planet_xyz_eye_xyz = planet_xyz_ + eye_xyz_;
  float y_y0_z_z0 = planet_y_ * y0 + planet_z_ * z0;
  float oowidth = 1.0f / width_;
  uint32_t *pixels = this->wGetAddr(x0, y);
  for (int x = x0; x <= x1; ++x) {
    // scan normalized screen -1..1
    float x1 = (static_cast<float>(x) * oowidth) * 2.0f - 1.0f;
    // eye
    float x0 = eye_x_;
    // delta from screen to eye
    float dx = (x1 - x0);
    // build a, b, c
    float a = dx * dx + dy_dy_dz_dz;
    float b = 2.0f * dx * (x0 - planet_x_) + two_dy_y0_y_two_dz_z0_z;
    float c = planet_xyz_eye_xyz +
              -2.0f * (planet_x_ * x0 + y_y0_z_z0) - (planet_radius2_);
    // calculate discriminant
    float disc = b * b - 4.0f * a * c;

    // Did ray hit the sphere?
    if (disc < 0.0f) {
      *pixels = kColorBlack;
      ++pixels;
      continue;
    }

    // calc parametric t value
    float t = (-b - inline_sqrt(disc)) / (2.0f * a);
    float px = x0 + t * dx;
    float py = y0 + t * dy;
    float pz = z0 + t * dz;
    float nx = (px - planet_x_) * planet_one_over_radius_;
    float ny = (py - planet_y_) * planet_one_over_radius_;
    float nz = (pz - planet_z_) * planet_one_over_radius_;

    float Lx = (light_x_ - px);
    float Ly = (light_y_ - py);
    float Lz = (light_z_ - pz);
    float Lq = 1.0f / inline_quick_sqrt(Lx * Lx + Ly * Ly + Lz * Lz);
    Lx *= Lq;
    Ly *= Lq;
    Lz *= Lq;
    float d = (Lx * nx + Ly * ny + Lz * nz);
    float pr = (diffuse_r_ * d) + ambient_r_;
    float pg = (diffuse_g_ * d) + ambient_g_;
    float pb = (diffuse_b_ * d) + ambient_b_;
    float ds = -(nx * planet_pole_x_ +
                 ny * planet_pole_y_ +
                 nz * planet_pole_z_);
    float ang = acos_.TableLerp(ds);
    float v = ang * kOneOverPI;
    float dp = planet_equator_x_ * nx +
               planet_equator_y_ * ny +
               planet_equator_z_ * nz;
    float w = dp / sin(ang);
    if (w > 1.0f) w = 1.0f;
    if (w < -1.0f) w = -1.0f;
    float th = acos_.TableLerp(w) * kOneOver2PI;
    float dps = planet_pole_x_equator_x_ * nx +
                planet_pole_x_equator_y_ * ny +
                planet_pole_x_equator_z_ * nz;
    float u;
    if (dps < 0.0f)
      u = th;
    else
      u = 1.0f - th;

    int tx = static_cast<int>(u * base_tex_->width);
    int ty = static_cast<int>(v * base_tex_->height);
    int offset = tx + ty * base_tex_->width;
    uint32_t base_texel = base_tex_->pixels[offset];
    float tr = ExtractR(base_texel);
    float tg = ExtractG(base_texel);
    float tb = ExtractB(base_texel);

    float ipr = 1.0f - pr;
    if (ipr < 0.0f) ipr = 0.0f;
    float ipg = 1.0f - pg;
    if (ipg < 0.0f) ipg = 0.0f;
    float ipb = 1.0f - pb;
    if (ipb < 0.0f) ipb = 0.0f;

    int nix = static_cast<int>(u * night_tex_->width);
    int niy = static_cast<int>(v * night_tex_->height);
    int noffset = nix + niy * night_tex_->width;
    uint32_t night_texel = night_tex_->pixels[noffset];
    float nr = ExtractR(night_texel);
    float ng = ExtractG(night_texel);
    float nb = ExtractB(night_texel);
    
    unsigned int ir = Clamp255(pr * tr + nr * ipr);
    unsigned int ig = Clamp255(pg * tg + ng * ipg);
    unsigned int ib = Clamp255(pb * tb + nb * ipb);

    unsigned int color = MakeRGBA(ir, ig, ib, 0xFF);

    *pixels = color;
    ++pixels;
  }
}

// Renders a rectangular area of the screen, scan line at a time
void Planet::wRenderRect(int x, int y, int w, int h) {
  for (int j = y; j < (y + h); ++j) {
    this->wRenderPixelSpan(x, x + w - 1, j);
  }
}

// If multithreading, this function is only called by the worker threads.
void Planet::wRenderRegion(int region) {
  // convert region # into x0, y0, x1, y1 rectangle
  int x, y, w, h;
  wMakeRect(region, &x, &y, &w, &h);
  // render this rectangle
  wRenderRect(x, y, w, h);
}

// Entry point for worker thread.  Can't pass a member function around, so we
// have to do this little round-about.
void Planet::wRenderRegionEntry(int region, void* thiz) {
  static_cast<Planet*>(thiz)->wRenderRegion(region);
}

// Renders the planet, dispatching the work to multiple threads.
// Note: This Dispatch() is from the main PPAPI thread, so care must be taken
// not to attempt PPAPI calls from the worker threads, since Dispatch() will
// block here until all work is complete.  The worker threads are compute only
// and do not make any PPAPI calls.
void Planet::Render() {
  workers_->Dispatch(num_regions_, wRenderRegionEntry, this);
}

// Pre-calculations to make inner loops faster.
void Planet::CacheCalcs() {
  planet_xyz_ = planet_x_ * planet_x_ +
                planet_y_ * planet_y_ +
                planet_z_ * planet_z_;
  planet_radius2_ = planet_radius_ * planet_radius_;
  planet_one_over_radius_ = 1.0f / planet_radius_;
  eye_xyz_ = eye_x_ * eye_x_ + eye_y_ * eye_y_ + eye_z_ * eye_z_;
  // spin vector from center->equator
  planet_equator_x_ = cos(planet_spin_x_);
  planet_equator_y_ = 0.0f;
  planet_equator_z_ = sin(planet_spin_x_);

  // cache cross product of pole & equator
  planet_pole_x_equator_x_ = planet_pole_y_ * planet_equator_z_ -
                             planet_pole_z_ * planet_equator_y_;
  planet_pole_x_equator_y_ = planet_pole_z_ * planet_equator_x_ -
                             planet_pole_x_ * planet_equator_z_;
  planet_pole_x_equator_z_ = planet_pole_x_ * planet_equator_y_ -
                             planet_pole_y_ * planet_equator_x_;
}

void Planet::SetPlanetXYZR(float x, float y, float z, float r) {
  planet_x_ = x;
  planet_y_ = y;
  planet_z_ = z;
  planet_radius_ = r;
  CacheCalcs();
}

void Planet::SetEyeXYZ(float x, float y, float z) {
  eye_x_ = x;
  eye_y_ = y;
  eye_z_ = z;
  CacheCalcs();
}

void Planet::SetLightXYZ(float x, float y, float z) {
  light_x_ = x;
  light_y_ = y;
  light_z_ = z;
  CacheCalcs();
}

void Planet::SetAmbientRGB(float r, float g, float b) {
  ambient_r_ = r;
  ambient_g_ = g;
  ambient_b_ = b;
  CacheCalcs();
}

void Planet::SetDiffuseRGB(float r, float g, float b) {
  diffuse_r_ = r;
  diffuse_g_ = g;
  diffuse_b_ = b;
  CacheCalcs();
}

void Planet::SetPlanetPole(float x, float y, float z) {
  planet_pole_x_ = x;
  planet_pole_y_ = y;
  planet_pole_z_ = z;
  CacheCalcs();
}

void Planet::SetPlanetEquator(float x, float y, float z) {
  // This is really over-ridden by spin at the momenent.
  planet_equator_x_ = x;
  planet_equator_y_ = y;
  planet_equator_z_ = z;
  CacheCalcs();
}

void Planet::SetPlanetSpin(float x, float y) {
  planet_spin_x_ = x;
  planet_spin_y_ = y;
  CacheCalcs();
}

// Run a simple sim to spin the planet.  Update loop is run once per frame.
// Called from the main thread only and only when the worker threads are idle.
void Planet::UpdateSim() {
  float x = planet_spin_x_ + ui_spin_x_;
  float y = planet_spin_y_ + ui_spin_y_;
  // keep in nice range
  if (x > (kPI * 2.0f))
    x = x - kPI * 2.0f;
  if (y > (kPI * 2.0f))
    y = y - kPI * 2.0f;
  SetPlanetSpin(x, y);
}

void Planet::DidChangeView(const pp::Rect& position, const pp::Rect& clip) {
  if (position.size().width() == width_ &&
      position.size().height() == height_)
    return;  // Size didn't change, no need to update anything.
  // Create a new device context with the new size.
  DestroyContext();
  CreateContext(position.size());
  Update();
}

void Planet::StartBenchmark() {
  // For more consistent benchmark numbers, reset to default state.
  Reset();
  printf("Benchmark started...\n");
  benchmark_frame_counter_ = kFramesToBenchmark;
  benchmarking_ = true;
  benchmark_start_time_ = getseconds();
}

void Planet::EndBenchmark() {
  benchmark_end_time_ = getseconds();
  printf("Benchmark ended... time: %2.5f\n",
      benchmark_end_time_ - benchmark_start_time_);
  benchmarking_ = false;
  benchmark_frame_counter_ = 0;
  double total_time = benchmark_end_time_ - benchmark_start_time_;
  std::ostringstream message;
  message << "result: " << total_time;
  PostMessage(pp::Var(message.str()));
}

void Planet::SetZoom(float zoom, bool send_message) {
  ui_zoom_ = std::min(kZoomMax, std::max(kZoomMin, zoom));
  SetEyeXYZ(0.0f, 0.0f, -ui_zoom_);
  if (send_message) {
    std::ostringstream message;
    message << "zoom: " << ui_zoom_;
    PostMessage(pp::Var(message.str()));
  }
}

void Planet::SetLight(float light, bool send_message) {
  ui_light_ = std::min(kLightMax, std::max(kLightMin, light));
  SetDiffuseRGB(0.8f * ui_light_, 0.8f * ui_light_, 0.8f * ui_light_);
  SetAmbientRGB(0.4f * ui_light_, 0.4f * ui_light_, 0.4f * ui_light_);
  if (send_message) {
    std::ostringstream message;
    message << "light: " << ui_light_;
    PostMessage(pp::Var(message.str()));
  }
}

// Handle input events from the user.
bool Planet::HandleInputEvent(const pp::InputEvent& event) {
  switch (event.GetType()) {
    case PP_INPUTEVENT_TYPE_KEYDOWN: {
      pp::KeyboardInputEvent key = pp::KeyboardInputEvent(event);
      uint32_t key_code = key.GetKeyCode();
      if (key_code == 84)  // 't' key
        if (!benchmarking_)
          StartBenchmark();
      break;
    }
    case PP_INPUTEVENT_TYPE_MOUSEMOVE: {
      pp::MouseInputEvent mouse = pp::MouseInputEvent(event);
      if (mouse.GetModifiers() & PP_INPUTEVENT_MODIFIER_LEFTBUTTONDOWN) {
        PP_Point delta = mouse.GetMovement();
        float spin_x = std::min(4.0f, std::max(-4.0f, (float) delta.x * 0.5f));
        float spin_y = std::min(4.0f, std::max(-4.0f, (float) delta.y * 0.5f));
        ui_spin_x_ = spin_x / 100.0f;
        ui_spin_y_ = spin_y / 100.0f;
      }
      break;
    }
    case PP_INPUTEVENT_TYPE_WHEEL: {
      pp::WheelInputEvent wheel = pp::WheelInputEvent(event);
      PP_FloatPoint ticks = wheel.GetTicks();
      SetZoom(ui_zoom_ + (ticks.x + ticks.y) * kWheelSpeed, true);
      break;
    }
    default:
      return false;
  }
  return true;
}

// Handle messages sent from Javascript.
void Planet::HandleMessage(const pp::Var& message) {
  if (message.is_string()) {
    std::string message_string = message.AsString();
    if (message_string == "run benchmark" && !benchmarking_)
      StartBenchmark();
    else if (strstr(message_string.c_str(), "threads:")) {
      int thread_count = atoi(strstr(message_string.c_str(), " "));
      delete workers_;
      workers_ = new ThreadPool(thread_count);
    } else if (strstr(message_string.c_str(), "zoom:")) {
      SetZoom(atof(strstr(message_string.c_str(), " ")), false);
    } else if (strstr(message_string.c_str(), "light:")) {
      double light = atof(strstr(message_string.c_str(), " "));
      SetLight(light, false);
    } else if (strstr(message_string.c_str(), "tex_width:")) {
      width_for_tex_ = atoi(strstr(message_string.c_str(), " "));
    } else if (strstr(message_string.c_str(), "tex_height:")) {
      height_for_tex_ = atoi(strstr(message_string.c_str(), " "));
    }
  } else if (message.is_array_buffer()) {
    // Decompressed image arriving from JS.
    pp::VarArrayBuffer array = pp::VarArrayBuffer(message);
    uint32_t* data = static_cast<uint32_t*>(array.Map());
    if (!base_tex_)
      base_tex_ = new Texture(width_for_tex_, height_for_tex_, data);
    else if (!night_tex_)
      night_tex_ = new Texture(width_for_tex_, height_for_tex_, data);
    array.Unmap();
  } else {
    printf("Handle message unknown type: %s\n", message.DebugString().c_str());
  }
}

void Planet::FlushCallback(void* thiz, int32_t result) {
  static_cast<Planet*>(thiz)->Update();
}

// Update the 2d region and flush to make it visible on the page.
void Planet::FlushPixelBuffer() {
  graphics_2d_context_->PaintImageData(*image_data_, pp::Point(0, 0));
  graphics_2d_context_->Flush(pp::CompletionCallback(&FlushCallback, this));
}

void Planet::Update() {
  static double frame_time[kFrameTimeBufferSize];
  static double planet_time[kFrameTimeBufferSize];
  double start_time = getseconds();

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

void Planet::CreateContext(const pp::Size& size) {
  static bool first = true;
  if (first) {
    // Request the image from JS
    PostMessage("request_image: earth1k.jpg");
    PostMessage("request_image: earthnight1k.jpg");
    first = false;
  }
  graphics_2d_context_ = new pp::Graphics2D(this, size, false);
  if (!BindGraphics(*graphics_2d_context_))
    printf("Couldn't bind the device context\n");
  image_data_ = new pp::ImageData(this,
                                  PP_IMAGEDATAFORMAT_BGRA_PREMUL,
                                  size,
                                  false);
  width_ = image_data_->size().width();
  height_ = image_data_->size().height();
  stride_in_pixels_ = static_cast<uint32_t>(image_data_->stride() / 4);
  pixel_buffer_ = static_cast<uint32_t*>(image_data_->data());
  num_regions_ = height_;
}

void Planet::DestroyContext() {
  delete graphics_2d_context_;
  delete image_data_;
  graphics_2d_context_ = NULL;
  image_data_ = NULL;
  width_ = 0;
  height_ = 0;
  stride_in_pixels_ = 0;
  pixel_buffer_ = NULL;
}

class PlanetModule : public pp::Module {
 public:
  PlanetModule() : pp::Module() {}
  virtual ~PlanetModule() {}

  // Create and return a Planet instance.
  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new Planet(instance);
  }
};

namespace pp {
Module* CreateModule() {
  return new PlanetModule();
}
}  // namespace pp

