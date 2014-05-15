// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdio>
#include <cstdlib>

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/ppb_instance.h"

#include "tests/ppapi_websocket/module.h"

// Global PPP functions --------------------------------------------------------

PP_EXPORT int32_t PPP_InitializeModule(PP_Module module_id,
                                       PPB_GetInterface get_browser_interface) {
  printf("--- PPP_InitializeModule\n");
  fflush(stdout);

  Module* module = Module::Create(module_id, get_browser_interface);
  if (NULL == module)
    return PP_ERROR_FAILED;
  return PP_OK;
}

PP_EXPORT void PPP_ShutdownModule() {
  printf("--- PPP_ShutdownModule\n");
  fflush(stdout);
  Module::Free();
}

PP_EXPORT const void* PPP_GetInterface(const char* interface_name) {
  printf("--- PPP_GetInterface(%s)\n", interface_name);
  fflush(stdout);
  Module* module = Module::Get();
  if (NULL == module)
    return NULL;
  return module->GetPluginInterface(interface_name);
}
