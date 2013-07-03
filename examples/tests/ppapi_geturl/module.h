// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef TESTS_PPAPI_GETURL_MODULE_H_
#define TESTS_PPAPI_GETURL_MODULE_H_

#include <string>

#include "ppapi/c/pp_module.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/ppb.h"
#include "ppapi/c/ppb_core.h"
#include "ppapi/c/ppb_messaging.h"
#include "ppapi/c/ppb_var.h"
#include "ppapi/c/ppp.h"

// ppapi_geturl example is deliberately using C PPAPI interface.
// C++ PPAPI layer has pp::Module wrapper class.
class Module {
 public:
  static Module* Create(PP_Module module_id,
                        PPB_GetInterface get_browser_interface);
  static Module* Get();
  static void Free();

  const void* GetPluginInterface(const char* interface_name);
  const void* GetBrowserInterface(const char* interface_name);
  PP_Module module_id() { return module_id_; }
  const PPB_Core* ppb_core_interface() const { return ppb_core_interface_; }
  const PPB_Messaging* ppb_messaging_interface() const {
    return ppb_messaging_interface_;
  }
  const PPB_Var* ppb_var_interface() const { return ppb_var_interface_; }

  static char* VarToCStr(const PP_Var& var);
  static std::string VarToStr(const PP_Var& var);
  static PP_Var StrToVar(const char* str);
  static PP_Var StrToVar(const std::string& str);
  static std::string ErrorCodeToStr(int32_t error_code);

  // Constructs a parameterized message string then uses messaging.PostMessage
  // to send this message back to the browser.  The receiving message handler
  // is defined in ppapi_geturl.html
  void ReportResult(PP_Instance pp_instance,
                    const char* url,
                    bool as_file,
                    const char* text,
                    bool success);
 private:
  PP_Module module_id_;
  PPB_GetInterface get_browser_interface_;
  const PPB_Core* ppb_core_interface_;
  const PPB_Messaging* ppb_messaging_interface_;
  const PPB_Var* ppb_var_interface_;

  Module(PP_Module module_id, PPB_GetInterface get_browser_interface);
  ~Module() { }
  Module(const Module&);
  void operator=(const Module&);
};

#endif  // TESTS_PPAPI_GETURL_MODULE_H_
