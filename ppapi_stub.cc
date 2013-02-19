#include <stdio.h>
#include <string.h>

#include "ppapi/c/pp_errors.h"

#include "ppapi/c/ppp.h"
#include "ppapi/c/ppb_console.h"
#include "ppapi/c/ppb_messaging.h"
#include "ppapi/c/ppb_var.h"

#include "ppapi/c/ppp.h"
#include "ppapi/c/ppp_instance.h"


extern "C" {
  extern void Console_Log(PP_Instance instance, PP_LogLevel level, struct PP_Var value);
  extern void Console_LogWithSource(PP_Instance instance, PP_LogLevel level, struct PP_Var source, struct PP_Var value);
}

static PPB_Console console_interface = {
  &Console_Log,
  &Console_LogWithSource
};


/*
void Var_AddRef(struct PP_Var var) {
}

void Var_Release(struct PP_Var var) {
}

struct PP_Var Var_VarFromUtf8(const char* data, uint32_t len) {
  struct PP_Var temp;
  printf("Converting.\n");
  return temp;
}

const char* Var_VarToUtf8(struct PP_Var var, uint32_t* len) {
  return NULL;
}
*/

extern "C" {
  extern void Var_AddRef(struct PP_Var var);
  extern void Var_Release(struct PP_Var var);
  extern struct PP_Var Var_VarFromUtf8(const char* data, uint32_t len);
  extern const char* Var_VarToUtf8(struct PP_Var var, uint32_t* len);
}

static PPB_Var var_interface = {
  Var_AddRef,
  Var_Release,
  Var_VarFromUtf8,
  Var_VarToUtf8
};

extern "C" {
  extern void Messaging_PostMessage(PP_Instance instance, struct PP_Var var);
}

static PPB_Messaging messaging_interface = {
  Messaging_PostMessage
};


const void* get_browser_interface_c(const char* interface_name) {
  printf("Get interface: %s\n", interface_name);
  if (strcmp(interface_name, PPB_CONSOLE_INTERFACE) == 0) {
    return &console_interface;
  } else if (strcmp(interface_name, PPB_MESSAGING_INTERFACE) == 0) {
    return &messaging_interface;
  } else if (strcmp(interface_name, PPB_VAR_INTERFACE) == 0) {
    return &var_interface;
  }
  return NULL;
}

int main() {
  int32_t status = PPP_InitializeModule(1, &get_browser_interface_c);
  if (status != PP_OK) {
    printf("Failed to initialize module.\n");
    return 1;
  }

  const PPP_Instance* instance_interface = (const PPP_Instance*)PPP_GetInterface(PPP_INSTANCE_INTERFACE);
  if (instance_interface == NULL) {
    printf("Failed to get instance interface.\n");
    return 1;
  }

  PP_Instance instance = 1;
  // TODO arguments.
  status = instance_interface->DidCreate(instance, 0, NULL, NULL);
  if (status != PP_TRUE) {
    printf("Failed to create instance.\n");
    goto cleanup;
  }

 cleanup:
  instance_interface->DidDestroy(instance);
  PPP_ShutdownModule();
  return 0;
}
