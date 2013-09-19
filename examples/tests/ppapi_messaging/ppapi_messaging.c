/*
 * Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>

//#include "native_client/src/include/portability.h"
//#include "native_client/src/shared/platform/nacl_check.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_module.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/ppb.h"
#include "ppapi/c/ppb_core.h"
#include "ppapi/c/ppb_instance.h"
#include "ppapi/c/ppb_messaging.h"
#include "ppapi/c/ppb_var.h"
#include "ppapi/c/ppp.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/ppp_messaging.h"

# define UNREFERENCED_PARAMETER(P) do { (void) P; } while (0)
# define CHECK(P) do { (void) P; } while (0)

/* Global variables */
PPB_GetInterface get_browser_interface_func = NULL;
PP_Instance instance = 0;
PP_Module module = 0;

struct MessageInfo {
  PP_Instance instance;
  struct PP_Var message;
};

static PPB_Var* GetPPB_Var() {
  return (PPB_Var*)(*get_browser_interface_func)(PPB_VAR_INTERFACE);
}

static void SendOnMessageEventCallback(void* data, int32_t result) {
  struct MessageInfo* message_to_send = (struct MessageInfo*)data;
  PPB_Messaging* ppb_messaging =
      (PPB_Messaging*)(*get_browser_interface_func)(
          PPB_MESSAGING_INTERFACE);

  UNREFERENCED_PARAMETER(result);
  CHECK(ppb_messaging);

  ppb_messaging->PostMessage(message_to_send->instance,
                             message_to_send->message);

  PPB_Var* ppb_var = GetPPB_Var();
  ppb_var->Release(message_to_send->message);

  free(message_to_send);
}

/* TODO(dspringer): We need to add a test that calls PostMessage directly from
 * HandleMessage to ensure that this is all asynchronous.
 */
void HandleMessage(PP_Instance instance, struct PP_Var message) {
  PPB_Core* ppb_core =
      (PPB_Core*)(*get_browser_interface_func)(PPB_CORE_INTERFACE);
  struct MessageInfo* message_to_send = malloc(sizeof(struct MessageInfo));
  message_to_send->instance = instance;
  message_to_send->message = message;

  /* Echo message back to browser */
  ppb_core->CallOnMainThread(
      0,  /* I don't care about delay */
      PP_MakeCompletionCallback(SendOnMessageEventCallback, message_to_send),
      PP_OK);  /* Dummy value for result */
}

PP_Bool DidCreate(PP_Instance instance,
                  uint32_t argc,
                  const char** argn,
                  const char** argv) {
  UNREFERENCED_PARAMETER(instance);
  UNREFERENCED_PARAMETER(argc);
  UNREFERENCED_PARAMETER(argn);
  UNREFERENCED_PARAMETER(argv);
  return PP_TRUE;
}

void DidDestroy(PP_Instance instance) {
  UNREFERENCED_PARAMETER(instance);
}

void DidChangeView(PP_Instance instance,
                   PP_Resource resource) {
  UNREFERENCED_PARAMETER(instance);
  UNREFERENCED_PARAMETER(resource);
}

void DidChangeFocus(PP_Instance instance,
                    PP_Bool has_focus) {
  UNREFERENCED_PARAMETER(instance);
  UNREFERENCED_PARAMETER(has_focus);
}

static PP_Bool HandleDocumentLoad(PP_Instance instance,
                                  PP_Resource url_loader) {
  UNREFERENCED_PARAMETER(instance);
  UNREFERENCED_PARAMETER(url_loader);
  return PP_TRUE;
}

/* Implementations of the PPP entry points expected by the browser. */
PP_EXPORT int32_t PPP_InitializeModule(PP_Module module_id,
                                       PPB_GetInterface get_browser_interface) {
  module = module_id;
  get_browser_interface_func = get_browser_interface;
  return PP_OK;
}

PP_EXPORT void PPP_ShutdownModule() {
}

PP_EXPORT const void* PPP_GetInterface(const char* interface_name) {
  if (0 == strncmp(PPP_INSTANCE_INTERFACE, interface_name,
                   strlen(PPP_INSTANCE_INTERFACE))) {
    static PPP_Instance instance_interface = {
      DidCreate,
      DidDestroy,
      DidChangeView,
      DidChangeFocus,
      HandleDocumentLoad
    };
    return &instance_interface;
  } else if (0 == strncmp(PPP_MESSAGING_INTERFACE, interface_name,
                          strlen(PPP_MESSAGING_INTERFACE))) {
    static PPP_Messaging messaging_interface = {
      HandleMessage
    };
    return &messaging_interface;
  }
  return NULL;
}
