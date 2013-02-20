#include <stdio.h>
#include <string.h>

#include "ppapi/c/pp_errors.h"

#include "ppapi/c/ppp.h"
#include "ppapi/c/ppb_core.h"
#include "ppapi/c/ppb_console.h"
#include "ppapi/c/ppb_messaging.h"
#include "ppapi/c/ppb_var.h"

#include "ppapi/c/ppp.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/ppp_messaging.h"

extern "C" {
  extern void NotImplemented();
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
  NotImplemented();
}

void Core_ReleaseResource(PP_Resource resource) {
  NotImplemented();
}

PP_Time Core_GetTime() {
  NotImplemented();
  return 0;
}

PP_TimeTicks Core_GetTimeTicks() {
  NotImplemented();
  return 0;
}

void Core_CallOnMainThread(int32_t delay_in_milliseconds, struct PP_CompletionCallback callback, int32_t result) {
  NotImplemented();
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
  } else if (strcmp(interface_name, PPB_VAR_INTERFACE) == 0) {
    return &var_interface;
  } else if (strcmp(interface_name, PPB_VAR_INTERFACE_1_0) == 0) {
    return &var_interface_1_0;
  }
  printf("STUB not supported: %s\n", interface_name);
  return NULL;
}

void doPostMessage(PP_Instance instance, const char* message) {
  const PPP_Messaging* messaging_interface = (const PPP_Messaging*)PPP_GetInterface(PPP_MESSAGING_INTERFACE);
  if (!messaging_interface) {
    return;
  }
  PP_Var message_var = var_interface.VarFromUtf8(message, strlen(message));
  messaging_interface->HandleMessage(instance, message_var);
  var_interface.Release(message_var);
}

int main() {
  int32_t status = PPP_InitializeModule(1, &get_browser_interface_c);
  if (status != PP_OK) {
    printf("STUB: Failed to initialize module.\n");
    return 1;
  }

  const PPP_Instance* instance_interface = (const PPP_Instance*)PPP_GetInterface(PPP_INSTANCE_INTERFACE);
  if (instance_interface == NULL) {
    printf("STUB: Failed to get instance interface.\n");
    return 1;
  }

  const PP_Instance instance = 1;

  // TODO arguments.
  status = instance_interface->DidCreate(instance, 0, NULL, NULL);
  if (status != PP_TRUE) {
    printf("STUB: Failed to create instance.\n");
    goto cleanup;
  }

  doPostMessage(instance, "getUrl:hello_world.html");

 cleanup:
  instance_interface->DidDestroy(instance);
  PPP_ShutdownModule();
  return 0;
}
