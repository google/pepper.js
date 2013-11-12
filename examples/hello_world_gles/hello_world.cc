/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/** @file hello_world_gles.cc
 * This example demonstrates loading and running a simple 3D openGL ES 2.0
 * application.
 */

//-----------------------------------------------------------------------------
// The spinning Cube
//-----------------------------------------------------------------------------

#define _USE_MATH_DEFINES 1
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_graphics_3d.h"
#include "ppapi/c/pp_module.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/ppb.h"
#include "ppapi/c/ppb_core.h"
#include "ppapi/c/ppb_graphics_3d.h"
#include "ppapi/c/ppb_fullscreen.h"
#include "ppapi/c/ppb_instance.h"
#include "ppapi/c/ppb_input_event.h"
#include "ppapi/c/ppb_messaging.h"
#include "ppapi/c/ppb_opengles2.h"
#include "ppapi/c/ppb_var.h"
#include "ppapi/c/ppb_view.h"
#include "ppapi/c/ppp.h"
#include "ppapi/c/ppp_input_event.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/ppp_messaging.h"
#include "ppapi/c/ppb_url_loader.h"
#include "ppapi/c/ppb_url_request_info.h"

#include "ppapi/c/ppp_graphics_3d.h"
#include "ppapi/lib/gl/gles2/gl2ext_ppapi.h"

#include <GLES2/gl2.h>
#include "matrix.h"
static PPB_Messaging* ppb_messaging_interface = NULL;
static PPB_Var* ppb_var_interface = NULL;
static PPB_Core* ppb_core_interface = NULL;
static PPB_Fullscreen* ppb_fullscreen_interface = NULL;
static PPB_Graphics3D* ppb_g3d_interface = NULL;
static PPB_InputEvent* ppb_input_event_interface = NULL;
static PPB_Instance* ppb_instance_interface = NULL;
static PPB_URLRequestInfo* ppb_urlrequestinfo_interface = NULL;
static PPB_URLLoader* ppb_urlloader_interface = NULL;
static PPB_View* ppb_view_interface = NULL;

static PP_Instance g_instance;
static PP_Resource g_context;

int view_width = 640;
int view_height = 480;

GLuint  g_positionLoc;
GLuint  g_texCoordLoc;
GLuint  g_colorLoc;
GLuint  g_MVPLoc;
GLuint  g_vboID;
GLuint  g_ibID;
GLushort g_Indices[36];

GLuint g_programObj;
GLuint g_vertexShader;
GLuint g_fragmentShader;

GLuint g_textureLoc = 0;
GLuint g_textureID = 0;

float g_fSpinX = 0.0f;
float g_fSpinY = 0.0f;

//-----------------------------------------------------------------------------
// Rendering Assets
//-----------------------------------------------------------------------------
struct Vertex
{
  float tu, tv;
  float color[3];
  float loc[3];
};

Vertex *g_quadVertices = NULL;
const char *g_TextureData = NULL;
const char *g_VShaderData = NULL;
const char *g_FShaderData = NULL;
int g_LoadCnt = 0;

//-----------------------------------------------------------------------------
// PROTOTYPES
//-----------------------------------------------------------------------------
void PostMessage(const char *fmt, ...);
char* LoadFile(const char *fileName);

void BuildQuad(Vertex* verts, int axis[3], float depth, float color[3]);
Vertex* BuildCube(void);

void InitGL(void);
void InitProgram(void);
void Render(void);


static struct PP_Var CStrToVar(const char* str) {
  if (ppb_var_interface != NULL) {
    return ppb_var_interface->VarFromUtf8(str, strlen(str));
  }
  return PP_MakeUndefined();
}


void PostMessage(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char msg[4096];
  vsnprintf(msg, sizeof(msg), fmt, args);

  if (ppb_messaging_interface)
    ppb_messaging_interface->PostMessage(g_instance, CStrToVar(msg));

  va_end(args);
}

void MainLoop(void* foo, int bar) {
  if (g_LoadCnt == 3) {
    InitProgram();
    g_LoadCnt++;
  }
  if (g_LoadCnt > 3) {
    Render();
    PP_CompletionCallback cc = PP_MakeCompletionCallback(MainLoop, 0);
    ppb_g3d_interface->SwapBuffers(g_context, cc);
  } else {
    PP_CompletionCallback cc = PP_MakeCompletionCallback(MainLoop, 0);
    ppb_core_interface->CallOnMainThread(0, cc, 0);
  }
}

void InitGL(void)
{
  int32_t attribs[] = {
    PP_GRAPHICS3DATTRIB_ALPHA_SIZE, 8,
    PP_GRAPHICS3DATTRIB_DEPTH_SIZE, 24,
    PP_GRAPHICS3DATTRIB_STENCIL_SIZE, 8,
    PP_GRAPHICS3DATTRIB_SAMPLES, 0,
    PP_GRAPHICS3DATTRIB_SAMPLE_BUFFERS, 0,
    PP_GRAPHICS3DATTRIB_WIDTH, view_width,
    PP_GRAPHICS3DATTRIB_HEIGHT, view_height,
    PP_GRAPHICS3DATTRIB_NONE
  };

  g_context =  ppb_g3d_interface->Create(g_instance, 0, attribs);
  int32_t success = ppb_instance_interface->BindGraphics(g_instance, g_context);
  if (success == PP_FALSE)
  {
    glSetCurrentContextPPAPI(0);
    printf("Failed to set context.\n");
    return;
  }
  glSetCurrentContextPPAPI(g_context);

  glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
}


GLuint compileShader(GLenum type, const char *data) {
  const char *shaderStrings[1];
  shaderStrings[0] = data;

  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, shaderStrings, NULL );
  glCompileShader(shader);
  return shader;
}


void InitProgram( void )
{
  glSetCurrentContextPPAPI(g_context);

  g_vertexShader = compileShader(GL_VERTEX_SHADER, g_VShaderData);
  g_fragmentShader = compileShader(GL_FRAGMENT_SHADER, g_FShaderData);

  g_programObj = glCreateProgram();
  glAttachShader(g_programObj, g_vertexShader);
  glAttachShader(g_programObj, g_fragmentShader);
  glLinkProgram(g_programObj);

  glGenBuffers(1, &g_vboID);
  glBindBuffer(GL_ARRAY_BUFFER, g_vboID);
  glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(Vertex), (void*)&g_quadVertices[0],
               GL_STATIC_DRAW);

  glGenBuffers(1, &g_ibID);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ibID);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_Indices), (void*)&g_Indices[0],
               GL_STATIC_DRAW);

  //
  // Create a texture to test out our fragment shader...
  //
  glGenTextures(1, &g_textureID);
  glBindTexture(GL_TEXTURE_2D, g_textureID);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE,
               g_TextureData);

  //
  // Locate some parameters by name so we can set them later...
  //
  g_textureLoc = glGetUniformLocation(g_programObj, "arrowTexture");
  g_positionLoc = glGetAttribLocation(g_programObj, "a_position");
  g_texCoordLoc = glGetAttribLocation(g_programObj, "a_texCoord");
  g_colorLoc = glGetAttribLocation(g_programObj, "a_color");
  g_MVPLoc = glGetUniformLocation(g_programObj, "a_MVP");
}


void BuildQuad(Vertex* verts, int axis[3], float depth, float color[3]) {
  static float X[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
  static float Y[4] = { -1.0f, -1.0f, 1.0f, 1.0f };

  for (int i = 0; i < 4; i++) {
    verts[i].tu = (1.0 - X[i]) / 2.0f;
    verts[i].tv = (Y[i] + 1.0f) / -2.0f * depth;
    verts[i].loc[axis[0]] = X[i] * depth;
    verts[i].loc[axis[1]] = Y[i] * depth;
    verts[i].loc[axis[2]] = depth;
    for (int j = 0; j < 3; j++)
      verts[i].color[j] = color[j] * (Y[i] + 1.0f) / 2.0f;
  }
}


Vertex *BuildCube() {
  Vertex *verts = new Vertex[24];
  for (int i = 0; i < 3; i++) {
    int Faxis[3];
    int Baxis[3];
    float Fcolor[3];
    float Bcolor[3];
    for (int j = 0; j < 3; j++) {
      Faxis[j] = (j + i) % 3;
      Baxis[j] = (j + i) % 3;
    }
    memset(Fcolor, 0, sizeof(float) * 3);
    memset(Bcolor, 0, sizeof(float) * 3);
    Fcolor[i] = 0.5f;
    Bcolor[i] = 1.0f;
    BuildQuad(&verts[0 + i * 4], Faxis, 1.0f, Fcolor);
    BuildQuad(&verts[12 + i * 4], Baxis, -1.0f, Bcolor);
  }

  for(int i = 0; i < 6; i++) {
    g_Indices[i*6 + 0] = 2 + i * 4;
    g_Indices[i*6 + 1] = 1 + i * 4;
    g_Indices[i*6 + 2] = 0 + i * 4;
    g_Indices[i*6 + 3] = 3 + i * 4;
    g_Indices[i*6 + 4] = 2 + i * 4;
    g_Indices[i*6 + 5] = 0 + i * 4;
  }
  return verts;
}


void Render( void )
{
  static float xRot = 0.0;
  static float yRot = 0.0;

  xRot += 2.0f;
  yRot += 0.5f;
  if (xRot >= 360.0f) xRot = 0.0;
  if (yRot >= 360.0f) yRot = 0.0;

  glViewport(0,0, view_width, view_height);
  glClearColor(0.5,0.5,0.5,1);
  glClearDepthf(1.0);
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  glEnable(GL_DEPTH_TEST);

  //set what program to use
  glUseProgram( g_programObj );
  glActiveTexture ( GL_TEXTURE0 );
  glBindTexture ( GL_TEXTURE_2D,g_textureID );
  glUniform1i ( g_textureLoc, 0 );

  //create our perspective matrix
  float mpv[16];
  float trs[16];
  float rot[16];

  identity_matrix(mpv);
  glhPerspectivef2(&mpv[0], 45.0f, view_width / (float) view_height, 1, 10);

  translate_matrix(0, 0, -4.0, trs);
  rotate_matrix(xRot, yRot , 0.0f ,rot);
  multiply_matrix(trs, rot, trs);
  multiply_matrix(mpv, trs, mpv);
  glUniformMatrix4fv(g_MVPLoc, 1, GL_FALSE, (GLfloat*) mpv);

  //define the attributes of the vertex
  glBindBuffer(GL_ARRAY_BUFFER, g_vboID);
  glVertexAttribPointer(g_positionLoc, 3, GL_FLOAT, GL_FALSE,
                        sizeof(Vertex), (void*)offsetof(Vertex,loc));
  glEnableVertexAttribArray(g_positionLoc);
  glVertexAttribPointer(g_texCoordLoc, 2, GL_FLOAT, GL_FALSE,
                        sizeof(Vertex), (void*)offsetof(Vertex,tu));
  glEnableVertexAttribArray(g_texCoordLoc);
  glVertexAttribPointer(g_colorLoc, 3, GL_FLOAT, GL_FALSE,
                        sizeof(Vertex), (void*)offsetof(Vertex,color));
  glEnableVertexAttribArray(g_colorLoc);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ibID);
  glDrawElements ( GL_TRIANGLES, 36, GL_UNSIGNED_SHORT ,0 );
}


typedef void (*OpenCB)(void *dataPtr);
struct OpenRequest {
  PP_Resource loader_;
  PP_Resource request_;
  char* buf_;
  void* data_;
  int64_t size_;
  int64_t avail_;
  OpenCB cb_;
};


void FreeRequest(OpenRequest* req) {
  if (req) {
    ppb_core_interface->ReleaseResource(req->request_);
    ppb_core_interface->ReleaseResource(req->loader_);
    free(req);
  }
}


static void URLPartialRead(void* user_data, int mode) {
  OpenRequest* req = (OpenRequest *) user_data;
  int64_t total;
  int32_t cnt;

  if (mode < 0) {
    free(req->buf_);
    req->cb_(NULL);
    FreeRequest(req);
    return;
  }

  req->avail_ += mode;
  total = req->size_ - req->avail_;

  cnt = (total > LONG_MAX) ? LONG_MAX : (int32_t) total;
  // If we still have more to do, re-issue the read.
  if (cnt > 0) {
    int32_t bytes = ppb_urlloader_interface->ReadResponseBody(req->loader_,
        (void *) &req->buf_[req->avail_], cnt,
        PP_MakeCompletionCallback(URLPartialRead, req));

    // If the reissue completes immediately, then process it.
    if (bytes != PP_OK_COMPLETIONPENDING) {
      URLPartialRead(user_data, bytes);
    }
    return;
  }

  // Nothing left, so signal complete.
  req->cb_(req);
  printf("Loaded %d\n", (int) req->size_);
  FreeRequest(req);
}


static void URLOpened(void* user_data, int mode) {
  OpenRequest* req = (OpenRequest *) user_data;

  int64_t cur, total;
  int32_t cnt;
  ppb_urlloader_interface->GetDownloadProgress(req->loader_, &cur, &total);

  // If we can't preallocate the buffer because the size is unknown, then
  // fail the load.
  if (total == -1) {
    req->cb_(NULL);
    FreeRequest(req);
    return;
  }

  // Otherwise allocate a buffer with enough space for a terminating
  // NUL in case we need one.
  cnt = (total > LONG_MAX) ? LONG_MAX : (int32_t) total;
  req->buf_ = (char *) malloc(cnt + 1);
  req->buf_[cnt] = 0;
  req->size_ = cnt;
  int32_t bytes = ppb_urlloader_interface->ReadResponseBody(req->loader_,
      req->buf_, cnt, PP_MakeCompletionCallback(URLPartialRead, req));

  // Usually we are pending.
  if (bytes == PP_OK_COMPLETIONPENDING) return;

  // But if we did complete the read, then dispatch the handler.
  URLPartialRead(req, bytes);
}

void LoadURL(PP_Instance inst, const char *url, OpenCB cb, void *data) {
  OpenRequest* req = (OpenRequest*) malloc(sizeof(OpenRequest));
  memset(req, 0, sizeof(OpenRequest));

  req->loader_ = ppb_urlloader_interface->Create(inst);
  req->request_ = ppb_urlrequestinfo_interface->Create(inst);
  req->cb_ = cb;
  req->data_ = data;

  if (!req->loader_ || !req->request_) {
    cb(NULL);
    FreeRequest(req);
    return;
  }

  ppb_urlrequestinfo_interface->SetProperty(req->request_,
      PP_URLREQUESTPROPERTY_URL, CStrToVar(url));
  ppb_urlrequestinfo_interface->SetProperty(req->request_,
      PP_URLREQUESTPROPERTY_METHOD, CStrToVar("GET"));
  ppb_urlrequestinfo_interface->SetProperty(req->request_,
      PP_URLREQUESTPROPERTY_RECORDDOWNLOADPROGRESS, PP_MakeBool(PP_TRUE));

  int val = ppb_urlloader_interface->Open(req->loader_, req->request_,
      PP_MakeCompletionCallback(URLOpened, req));

  if (val != PP_OK_COMPLETIONPENDING) {
    cb(NULL);
    free(req);
  }
}

void Loaded(void* data) {
  OpenRequest *req = (OpenRequest *) data;
  if (req && req->buf_) {
    char **pptr = (char **) req->data_;
    *pptr = req->buf_;
    g_LoadCnt++;
    return;
  }
  PostMessage("Failed to load asset.\n");
}


/**
 * Called when the NaCl module is instantiated on the web page. The identifier
 * of the new instance will be passed in as the first argument (this value is
 * generated by the browser and is an opaque handle).  This is called for each
 * instantiation of the NaCl module, which is each time the <embed> tag for
 * this module is encountered.
 *
 * If this function reports a failure (by returning @a PP_FALSE), the NaCl
 * module will be deleted and DidDestroy will be called.
 * @param[in] instance The identifier of the new instance representing this
 *     NaCl module.
 * @param[in] argc The number of arguments contained in @a argn and @a argv.
 * @param[in] argn An array of argument names.  These argument names are
 *     supplied in the <embed> tag, for example:
 *       <embed id="nacl_module" dimensions="2">
 *     will produce two arguments, one named "id" and one named "dimensions".
 * @param[in] argv An array of argument values.  These are the values of the
 *     arguments listed in the <embed> tag.  In the above example, there will
 *     be two elements in this array, "nacl_module" and "2".  The indices of
 *     these values match the indices of the corresponding names in @a argn.
 * @return @a PP_TRUE on success.
 */
static PP_Bool Instance_DidCreate(PP_Instance instance,
                                  uint32_t argc,
                                  const char* argn[],
                                  const char* argv[]) {
  g_instance = instance;
  LoadURL(instance, "hello.raw", Loaded, &g_TextureData);
  LoadURL(instance, "vertex_shader_es2.vert", Loaded, &g_VShaderData);
  LoadURL(instance, "fragment_shader_es2.frag", Loaded, &g_FShaderData);
  g_quadVertices = BuildCube();

  // Set up handler to go fullscreen on clicks
  ppb_input_event_interface->RequestInputEvents(instance, PP_INPUTEVENT_CLASS_MOUSE);
  return PP_TRUE;
}


/**
 * Called when the NaCl module is destroyed. This will always be called,
 * even if DidCreate returned failure. This routine should deallocate any data
 * associated with the instance.
 * @param[in] instance The identifier of the instance representing this NaCl
 *     module.
 */
static void Instance_DidDestroy(PP_Instance instance) {
  delete[] g_TextureData;
  delete[] g_VShaderData;
  delete[] g_FShaderData;
  delete[] g_quadVertices;
}

/**
 * Called when the position, the size, or the clip rect of the element in the
 * browser that corresponds to this NaCl module has changed.
 * @param[in] instance The identifier of the instance representing this NaCl
 *     module.
 * @param[in] position The location on the page of this NaCl module. This is
 *     relative to the top left corner of the viewport, which changes as the
 *     page is scrolled.
 * @param[in] clip The visible region of the NaCl module. This is relative to
 *     the top left of the plugin's coordinate system (not the page).  If the
 *     plugin is invisible, @a clip will be (0, 0, 0, 0).
 */
static void Instance_DidChangeView(PP_Instance instance,
                                   PP_Resource view_resource) {
  PP_Rect rect;
  ppb_view_interface->GetRect(view_resource, &rect);
  view_width = rect.size.width;
  view_height = rect.size.height;

  printf("View change: %d x %d\n", view_width, view_height);

  if (g_context == 0) {
    InitGL();
    MainLoop(NULL, 0);
  } else {
    ppb_g3d_interface->ResizeBuffers(g_context, view_width, view_height);
  }
}

/**
 * Notification that the given NaCl module has gained or lost focus.
 * Having focus means that keyboard events will be sent to the NaCl module
 * represented by @a instance. A NaCl module's default condition is that it
 * will not have focus.
 *
 * Note: clicks on NaCl modules will give focus only if you handle the
 * click event. You signal if you handled it by returning @a true from
 * HandleInputEvent. Otherwise the browser will bubble the event and give
 * focus to the element on the page that actually did end up consuming it.
 * If you're not getting focus, check to make sure you're returning true from
 * the mouse click in HandleInputEvent.
 * @param[in] instance The identifier of the instance representing this NaCl
 *     module.
 * @param[in] has_focus Indicates whether this NaCl module gained or lost
 *     event focus.
 */
static void Instance_DidChangeFocus(PP_Instance instance,
                                    PP_Bool has_focus) {
}

/**
 * Handler that gets called after a full-frame module is instantiated based on
 * registered MIME types.  This function is not called on NaCl modules.  This
 * function is essentially a place-holder for the required function pointer in
 * the PPP_Instance structure.
 * @param[in] instance The identifier of the instance representing this NaCl
 *     module.
 * @param[in] url_loader A PP_Resource an open PPB_URLLoader instance.
 * @return PP_FALSE.
 */
static PP_Bool Instance_HandleDocumentLoad(PP_Instance instance,
                                           PP_Resource url_loader) {
  /* NaCl modules do not need to handle the document load function. */
  return PP_FALSE;
}

static PP_Bool InputEvent_HandleInputEvent(PP_Instance instance, PP_Resource input_event) {

  PP_InputEvent_Type type = ppb_input_event_interface->GetType(input_event);
  if (type == PP_INPUTEVENT_TYPE_MOUSEUP) {
    if (ppb_fullscreen_interface) {
      ppb_fullscreen_interface->SetFullscreen(instance, PP_TRUE);
    }
    return PP_TRUE;
  }

  return PP_FALSE;
}


/**
 * Entry points for the module.
 * Initialize needed interfaces: PPB_Core, PPB_Messaging and PPB_Var.
 * @param[in] a_module_id module ID
 * @param[in] get_browser pointer to PPB_GetInterface
 * @return PP_OK on success, any other value on failure.
 */
PP_EXPORT int32_t PPP_InitializeModule(PP_Module a_module_id,
                                       PPB_GetInterface get_browser) {
  ppb_core_interface = (PPB_Core*)(get_browser(PPB_CORE_INTERFACE));
  ppb_instance_interface = (PPB_Instance*)get_browser(PPB_INSTANCE_INTERFACE);
  ppb_messaging_interface =
      (PPB_Messaging*)(get_browser(PPB_MESSAGING_INTERFACE));
  ppb_var_interface = (PPB_Var*)(get_browser(PPB_VAR_INTERFACE));
  ppb_urlloader_interface =
      (PPB_URLLoader*)(get_browser(PPB_URLLOADER_INTERFACE));
  ppb_urlrequestinfo_interface =
      (PPB_URLRequestInfo*)(get_browser(PPB_URLREQUESTINFO_INTERFACE));
  ppb_g3d_interface = (PPB_Graphics3D*)get_browser(PPB_GRAPHICS_3D_INTERFACE);

  ppb_fullscreen_interface = (PPB_Fullscreen*)get_browser(PPB_FULLSCREEN_INTERFACE);
  ppb_input_event_interface = (PPB_InputEvent*)get_browser(PPB_INPUT_EVENT_INTERFACE);

  ppb_view_interface = (PPB_View*)(get_browser(PPB_VIEW_INTERFACE));

  if (!glInitializePPAPI(get_browser))
    return PP_ERROR_FAILED;
  return PP_OK;
}


/**
 * Returns an interface pointer for the interface of the given name, or NULL
 * if the interface is not supported.
 * @param[in] interface_name name of the interface
 * @return pointer to the interface
 */
PP_EXPORT const void* PPP_GetInterface(const char* interface_name) {
  if (strcmp(interface_name, PPP_INSTANCE_INTERFACE) == 0) {
    static PPP_Instance instance_interface = {
      &Instance_DidCreate,
      &Instance_DidDestroy,
      &Instance_DidChangeView,
      &Instance_DidChangeFocus,
      &Instance_HandleDocumentLoad,
    };
    return &instance_interface;
  } else if (strcmp(interface_name, PPP_INPUT_EVENT_INTERFACE) == 0) {
    static PPP_InputEvent input_event_interface = {
      &InputEvent_HandleInputEvent
    };
    return &input_event_interface;
  }


  return NULL;
}


/**
 * Called before the plugin module is unloaded.
 */
PP_EXPORT void PPP_ShutdownModule() {
}
