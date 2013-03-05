#include <stdio.h>
#include <string.h>

#include "ppapi/c/pp_errors.h"

#include "ppapi/c/ppp.h"
#include "ppapi/c/ppb_core.h"
#include "ppapi/c/ppb_console.h"
#include "ppapi/c/ppb_messaging.h"
#include "ppapi/c/ppb_url_loader.h"
#include "ppapi/c/ppb_url_request_info.h"
#include "ppapi/c/ppb_url_response_info.h"
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
  extern void Console_Log(PP_Instance instance, PP_LogLevel level, struct PP_Var value);
  extern void Console_LogWithSource(PP_Instance instance, PP_LogLevel level, struct PP_Var source, struct PP_Var value);
}

static PPB_Console console_interface = {
  &Console_Log,
  &Console_LogWithSource
};


void Core_AddRefResource(PP_Resource resource) {
  ThrowNotImplemented();
}

extern "C" {
  extern void Core_ReleaseResource(PP_Resource resource);
}

PP_Time Core_GetTime() {
  ThrowNotImplemented();
  return 0;
}

PP_TimeTicks Core_GetTimeTicks() {
  ThrowNotImplemented();
  return 0;
}

void Core_CallOnMainThread(int32_t delay_in_milliseconds, struct PP_CompletionCallback callback, int32_t result) {
  ThrowNotImplemented();
}

PP_Bool Core_IsMainThread() {
  return PP_TRUE;
}

static PPB_Core core_interface = {
  &Core_AddRefResource,
  &Core_ReleaseResource,
  &Core_GetTime,
  &Core_GetTimeTicks,
  &Core_CallOnMainThread,
  &Core_IsMainThread
};

extern "C" {
  PP_Resource URLLoader_Create(PP_Instance instance);
  PP_Bool URLLoader_IsURLLoader(PP_Resource resource);
  int32_t URLLoader_Open(PP_Resource loader,
                  PP_Resource request_info,
                  struct PP_CompletionCallback callback);
  int32_t URLLoader_FollowRedirect(PP_Resource loader,
                            struct PP_CompletionCallback callback);
  PP_Bool URLLoader_GetUploadProgress(PP_Resource loader,
                               int64_t* bytes_sent,
                               int64_t* total_bytes_to_be_sent);
  PP_Bool URLLoader_GetDownloadProgress(PP_Resource loader,
                                 int64_t* bytes_received,
                                 int64_t* total_bytes_to_be_received);
  PP_Resource URLLoader_GetResponseInfo(PP_Resource loader);
  int32_t URLLoader_ReadResponseBody(PP_Resource loader,
                              void* buffer,
                              int32_t bytes_to_read,
                              struct PP_CompletionCallback callback);
  int32_t URLLoader_FinishStreamingToFile(PP_Resource loader,
                                   struct PP_CompletionCallback callback);
  void URLLoader_Close(PP_Resource loader);
}

static PPB_URLLoader url_loader_interface = {
  &URLLoader_Create,
  &URLLoader_IsURLLoader,
  &URLLoader_Open,
  &URLLoader_FollowRedirect,
  &URLLoader_GetUploadProgress,
  &URLLoader_GetDownloadProgress,
  &URLLoader_GetResponseInfo,
  &URLLoader_ReadResponseBody,
  &URLLoader_FinishStreamingToFile,
  &URLLoader_Close
};

extern "C" {
  PP_Resource URLRequestInfo_Create(PP_Instance instance);
  PP_Bool URLRequestInfo_IsURLRequestInfo(PP_Resource resource);
  PP_Bool URLRequestInfo_SetProperty(PP_Resource request,
                         PP_URLRequestProperty property,
                         struct PP_Var value);
  PP_Bool URLRequestInfo_AppendDataToBody(PP_Resource request,
                              const void* data,
                              uint32_t len);
  PP_Bool URLRequestInfo_AppendFileToBody(PP_Resource request,
                              PP_Resource file_ref,
                              int64_t start_offset,
                              int64_t number_of_bytes,
                              PP_Time expected_last_modified_time);
}

static PPB_URLRequestInfo url_request_info_interface = {
  &URLRequestInfo_Create,
  &URLRequestInfo_IsURLRequestInfo,
  &URLRequestInfo_SetProperty,
  &URLRequestInfo_AppendDataToBody,
  &URLRequestInfo_AppendFileToBody
};


extern "C" {
  extern void Var_AddRef(struct PP_Var var);
  extern void Var_Release(struct PP_Var var);
  extern struct PP_Var Var_VarFromUtf8(const char* data, uint32_t len);
  extern const char* Var_VarToUtf8(struct PP_Var var, uint32_t* len);
}

static struct PP_Var Var_VarFromUtf8_1_0(PP_Module module, const char* data, uint32_t len) {
  return Var_VarFromUtf8(data, len);
}

static PPB_Var var_interface = {
  Var_AddRef,
  Var_Release,
  Var_VarFromUtf8,
  Var_VarToUtf8
};

// Used by ppapi_cpp
static PPB_Var_1_0 var_interface_1_0 = {
  Var_AddRef,
  Var_Release,
  Var_VarFromUtf8_1_0,
  Var_VarToUtf8
};

extern "C" {
  extern void Messaging_PostMessage(PP_Instance instance, struct PP_Var var);
}

static PPB_Messaging messaging_interface = {
  Messaging_PostMessage
};


const void* get_browser_interface_c(const char* interface_name) {
  printf("STUB requested: %s\n", interface_name);
  if (strcmp(interface_name, PPB_CONSOLE_INTERFACE) == 0) {
    return &console_interface;
  } else if (strcmp(interface_name, PPB_CORE_INTERFACE) == 0) {
    return &core_interface;
  } else if (strcmp(interface_name, PPB_MESSAGING_INTERFACE) == 0) {
    return &messaging_interface;
  } else if (strcmp(interface_name, PPB_URLLOADER_INTERFACE) == 0) {
    return &url_loader_interface;
  } else if (strcmp(interface_name, PPB_URLREQUESTINFO_INTERFACE) == 0) {
    return &url_request_info_interface;
  } else if (strcmp(interface_name, PPB_VAR_INTERFACE) == 0) {
    return &var_interface;
  } else if (strcmp(interface_name, PPB_VAR_INTERFACE_1_0) == 0) {
    return &var_interface_1_0;
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
    PP_Var message_var = var_interface.VarFromUtf8(message, strlen(message));
    messaging_interface->HandleMessage(instance, message_var);
    var_interface.Release(message_var);
  }

  CALLED_FROM_JS void RunCompletionCallback(struct PP_CompletionCallback* cc, int32_t result) {
    PP_RunCompletionCallback(cc, result);
  }

  void Shutdown(PP_Instance instance) {
    const PPP_Instance* instance_interface = (const PPP_Instance*)PPP_GetInterface(PPP_INSTANCE_INTERFACE);
    if (instance_interface) {
      instance_interface->DidDestroy(instance);
    }
    PPP_ShutdownModule();
  }

  PP_Instance Startup() {
    int32_t status = PPP_InitializeModule(1, &get_browser_interface_c);
    if (status != PP_OK) {
      printf("STUB: Failed to initialize module.\n");
      return 0;
    }

    const PPP_Instance* instance_interface = (const PPP_Instance*)PPP_GetInterface(PPP_INSTANCE_INTERFACE);
    if (instance_interface == NULL) {
      printf("STUB: Failed to get instance interface.\n");
      return 0;
    }

    const PP_Instance instance = 1;

    // TODO arguments.
    status = instance_interface->DidCreate(instance, 0, NULL, NULL);
    if (status != PP_TRUE) {
      printf("STUB: Failed to create instance.\n");
      Shutdown(instance);
      return 0;
    }

    return instance;
  }
}
