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
#include "ppapi/c/ppb_var_array.h"
#include "ppapi/c/ppb_var_array_buffer.h"
#include "ppapi/c/ppb_var_dictionary.h"
#include "ppapi/c/ppp.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/ppp_messaging.h"

# define UNREFERENCED_PARAMETER(P) do { (void) P; } while (0)
# define CHECK(P) do { (void) P; } while (0)

/* Global variables */
PPB_GetInterface get_browser_interface_func = NULL;
PP_Instance instance = 0;
PP_Module module = 0;

static PPB_Messaging* ppb_messaging = NULL;
static PPB_Var* ppb_var = NULL;
static PPB_VarArray* ppb_var_array = NULL;
static PPB_VarArrayBuffer* ppb_var_array_buffer = NULL;
static PPB_VarDictionary* ppb_var_dictionary = NULL;

extern void HandleMessage(PP_Instance instance, struct PP_Var message);

void emptyArray(PP_Instance instance) {
  struct PP_Var result = ppb_var_array->Create();
  ppb_messaging->PostMessage(instance, result);
  ppb_var->Release(result);
}

void simpleArray(PP_Instance instance) {
  int i;
  struct PP_Var value;
  struct PP_Var result = ppb_var_array->Create();
  ppb_var_array->SetLength(result, 8);
  for (i = 0; i < 4; i++) {
    value = PP_MakeInt32((i + 1) * (i + 1));
    ppb_var_array->Set(result, i, value);
  }
  value = ppb_var->VarFromUtf8("foo", 3);
  ppb_var_array->Set(result, 4, value);
  ppb_var->Release(value);

  value = ppb_var_array->Get(result, 4);
  ppb_var_array->Set(result, 5, value);
  ppb_var->Release(value);

  value = PP_MakeInt32(ppb_var_array->GetLength(result));
  ppb_var_array->Set(result, 7, value);
  ppb_messaging->PostMessage(instance, result);
  ppb_var->Release(result);
}


void emptyDictionary(PP_Instance instance) {
  struct PP_Var result = ppb_var_dictionary->Create();
  ppb_messaging->PostMessage(instance, result);
  ppb_var->Release(result);
}

void emptyArrayBuffer(PP_Instance instance) {
  struct PP_Var result = ppb_var_array_buffer->Create(0);
  ppb_messaging->PostMessage(instance, result);
  ppb_var->Release(result);
}

void simpleArrayBuffer(PP_Instance instance) {
  struct PP_Var result = ppb_var_array_buffer->Create(10);
  uint32_t length;
  uint8_t* data;

  if (ppb_var_array_buffer->ByteLength(result, &length)) {
    data = (uint8_t*)ppb_var_array_buffer->Map(result);

    if (data) {
      int i;
      for (i = 0; i < length; ++i) {
        data[i] = i;
      }
      ppb_var_array_buffer->Unmap(result);
    }
  }

  ppb_messaging->PostMessage(instance, result);
  ppb_var->Release(result);
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

  ppb_messaging =
      (PPB_Messaging*)(get_browser_interface(PPB_MESSAGING_INTERFACE));

  ppb_var =
      (PPB_Var*)(get_browser_interface(PPB_VAR_INTERFACE));

  ppb_var_array =
      (PPB_VarArray*)(get_browser_interface(PPB_VAR_ARRAY_INTERFACE));

  ppb_var_array_buffer = (PPB_VarArrayBuffer*)(get_browser_interface(
      PPB_VAR_ARRAY_BUFFER_INTERFACE));

  ppb_var_dictionary =
      (PPB_VarDictionary*)(get_browser_interface(PPB_VAR_DICTIONARY_INTERFACE));

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
