#include <stdio.h>
#include <string.h>

#include "ppapi/c/pp_errors.h"

#include "ppapi/c/ppp.h"
#include "ppapi/c/ppb_var.h"

#include "ppapi/c/ppp.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/ppp_messaging.h"

#define CALLED_FROM_JS __attribute__((used))

extern "C" {
  const void* GetBrowserInterface(const char* interface_name);

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
    int32_t status = PPP_InitializeModule(1, &GetBrowserInterface);
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
