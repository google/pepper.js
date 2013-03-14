#include <stdio.h>
#include <string.h>

#include "ppapi/c/pp_errors.h"

#include "ppapi/c/ppp.h"
#include "ppapi/c/ppb_graphics_2d.h"
#include "ppapi/c/ppb_image_data.h"
#include "ppapi/c/ppb_var.h"

#include "ppapi/c/ppp.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/ppp_messaging.h"

#include "deplug.h"

#define CALLED_FROM_JS __attribute__((used))

extern "C" {
  extern void ThrowNotImplemented();
}

extern "C" {
  PP_Resource Graphics2D_Create(PP_Instance instance, const struct PP_Size *size, PP_Bool is_always_opaque);
  PP_Bool Graphics2D_IsGraphics2D(PP_Resource resource);
  PP_Bool Graphics2D_Describe(PP_Resource graphics_2d, struct PP_Size *size, PP_Bool *is_always_opqaue);
  void Graphics2D_PaintImageData(PP_Resource graphics_2d, PP_Resource image_data, const struct PP_Point *top_left, const struct PP_Rect *src_rect);
  void Graphics2D_Scroll(PP_Resource graphics_2d, const struct PP_Rect *clip_rect, const struct PP_Point *amount);
  void Graphics2D_ReplaceContents(PP_Resource graphics_2d, PP_Resource image_data);
  int32_t Graphics2D_Flush(PP_Resource graphics_2d, struct PP_CompletionCallback callback);
}

static PPB_Graphics2D graphics_2d_interface_1_0 = {
  Graphics2D_Create,
  Graphics2D_IsGraphics2D,
  Graphics2D_Describe,
  Graphics2D_PaintImageData,
  Graphics2D_Scroll,
  Graphics2D_ReplaceContents,
  Graphics2D_Flush
};

extern "C" {
  PP_ImageDataFormat ImageData_GetNativeImageDataFormat();
  PP_Bool ImageData_IsImageDataFormatSupported(PP_ImageDataFormat format);
  PP_Resource ImageData_Create(PP_Instance instance, PP_ImageDataFormat format, const struct PP_Size *size, PP_Bool init_to_zero);
  PP_Bool ImageData_IsImageData(PP_Resource image_data);
  PP_Bool ImageData_Describe(PP_Resource image_data, struct PP_ImageDataDesc *desc);
  void* ImageData_Map(PP_Resource image_data);
  void ImageData_Unmap(PP_Resource image_data);
}

static PPB_ImageData image_data_interface_1_0 = {
  ImageData_GetNativeImageDataFormat,
  ImageData_IsImageDataFormatSupported,
  ImageData_Create,
  ImageData_IsImageData,
  ImageData_Describe,
  ImageData_Map,
  ImageData_Unmap
};

extern "C" {
  void* GetBrowserInterface(const char* interface_name);
}

const void* get_browser_interface_c(const char* interface_name) {
  printf("STUB requested: %s\n", interface_name);
  void* interface = GetBrowserInterface(interface_name);
  if(interface) {
    return interface;
  }

  if (strcmp(interface_name, PPB_GRAPHICS_2D_INTERFACE_1_0) == 0) {
    return &graphics_2d_interface_1_0;
  } else if (strcmp(interface_name, PPB_IMAGEDATA_INTERFACE_1_0) == 0) {
    return &image_data_interface_1_0;
  }
  printf("STUB not supported: %s\n", interface_name);
  return NULL;
}

extern "C" {
  extern void Schedule(void (*)(void*, void*), void*, void*);
  CALLED_FROM_JS void RunScheduled(void (*func)(void*, void*), void* p0, void* p1) {
    func(p0, p1);
  }
}

void RunDaemon(void* f, void* param) {
  DaemonFunc func = (DaemonFunc)f;
  struct DaemonCallback callback = func(param);
  LaunchDaemon(callback.func, callback.param);
}

void LaunchDaemon(DaemonFunc func, void* param) {
  if (func)
    Schedule(&RunDaemon, (void *) func, param);
}

extern "C" {
  CALLED_FROM_JS void DoPostMessage(PP_Instance instance, const char* message) {
    const PPP_Messaging* messaging_interface = (const PPP_Messaging*)PPP_GetInterface(PPP_MESSAGING_INTERFACE);
    if (!messaging_interface) {
      return;
    }
    const PPB_Var* var_interface = (const PPB_Var*)GetBrowserInterface(PPB_VAR_INTERFACE);
    if (!var_interface) {
      return;
    }

    PP_Var message_var = var_interface->VarFromUtf8(message, strlen(message));
    messaging_interface->HandleMessage(instance, message_var);
    // It appears that the callee own the var, so no need to release it?
  }

  CALLED_FROM_JS void DoChangeView(PP_Instance instance, PP_Resource resource) {
    const PPP_Instance* instance_interface = (const PPP_Instance*)PPP_GetInterface(PPP_INSTANCE_INTERFACE);
    if (instance_interface == NULL) {
      printf("STUB: Failed to get instance interface.\n");
      return;
    }
    instance_interface->DidChangeView(instance, resource);
  }

  void Shutdown(PP_Instance instance) {
    const PPP_Instance* instance_interface = (const PPP_Instance*)PPP_GetInterface(PPP_INSTANCE_INTERFACE);
    if (instance_interface) {
      instance_interface->DidDestroy(instance);
    }
    PPP_ShutdownModule();
  }

  CALLED_FROM_JS void NativeCreateInstance(PP_Instance instance) {
    int32_t status = PPP_InitializeModule(1, &get_browser_interface_c);
    if (status != PP_OK) {
      printf("STUB: Failed to initialize module.\n");
      return;
    }

    const PPP_Instance* instance_interface = (const PPP_Instance*)PPP_GetInterface(PPP_INSTANCE_INTERFACE);
    if (instance_interface == NULL) {
      printf("STUB: Failed to get instance interface.\n");
      return;
    }

    // TODO arguments.
    status = instance_interface->DidCreate(instance, 0, NULL, NULL);
    if (status != PP_TRUE) {
      printf("STUB: Failed to create instance.\n");
      Shutdown(instance);
      return;
    }
  }
}
