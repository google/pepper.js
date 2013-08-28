/* Copyright (c) 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/ppb.h"
#include "ppapi/c/ppb_console.h"
#include "ppapi/c/ppb_instance.h"
#include "ppapi/c/ppb_var.h"
#include "ppapi/c/ppp.h"
#include "ppapi/c/ppp_instance.h"

#include "interfaces.h"

PPB_GetInterface get_browser_interface;

static PPB_Console* ppb_console_interface = NULL;
static PPB_Var* ppb_var_interface = NULL;

static void LogMessage(PP_Instance instance, PP_LogLevel level, const char *str) {
  PP_Var wrapped = ppb_var_interface->VarFromUtf8(str, strlen(str));
  ppb_console_interface->Log(instance, level, wrapped);
  ppb_var_interface->Release(wrapped);
}

static PP_Bool Instance_DidCreate(PP_Instance instance,
                                  uint32_t argc,
                                  const char* argn[],
                                  const char* argv[]) {
  char buffer[1024];
  for (int i = 0; interfaces[i]; i++) {
    bool ok = get_browser_interface(interfaces[i]) != NULL;
    snprintf(buffer, sizeof(buffer), "%s %d", interfaces[i], ok);
    LogMessage(instance, ok ? PP_LOGLEVEL_LOG : PP_LOGLEVEL_ERROR, buffer);
  }
  return PP_TRUE;
}

static void Instance_DidDestroy(PP_Instance instance) {
}

static void Instance_DidChangeView(PP_Instance instance,
                                   PP_Resource view_resource) {
}

static void Instance_DidChangeFocus(PP_Instance instance,
                                    PP_Bool has_focus) {
}

static PP_Bool Instance_HandleDocumentLoad(PP_Instance instance,
                                           PP_Resource url_loader) {
  return PP_FALSE;
}

PP_EXPORT int32_t PPP_InitializeModule(PP_Module a_module_id,
                                       PPB_GetInterface get_browser) {
  get_browser_interface = get_browser;
  ppb_console_interface =
      (PPB_Console*)(get_browser(PPB_CONSOLE_INTERFACE));
  ppb_var_interface = (PPB_Var*)(get_browser(PPB_VAR_INTERFACE));
  return PP_OK;
}

static PPP_Instance instance_interface = {
  &Instance_DidCreate,
  &Instance_DidDestroy,
  &Instance_DidChangeView,
  &Instance_DidChangeFocus,
  &Instance_HandleDocumentLoad,
};

PP_EXPORT const void* PPP_GetInterface(const char* interface_name) {
  if (strcmp(interface_name, PPP_INSTANCE_INTERFACE) == 0) {
    return &instance_interface;
  }
  return NULL;
}

PP_EXPORT void PPP_ShutdownModule() {
}
