// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tests/ppapi_websocket/module.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

//#include "native_client/src/include/nacl_macros.h"
//#include "native_client/src/include/portability.h"
//#include "native_client/src/shared/platform/nacl_check.h"
#include "tests/ppapi_websocket/web_socket_request.h"

#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/ppp_messaging.h"

namespace {

// These constants need to match their corresponding JavaScript values in
// ppapi_geturl.html.  The JavaScript variables are all upper-case; for example
// kTrueStringValue corresponds to TRUE_STRING_VALUE.
const char* const kLoadUrlMethodId = "loadWebSocket";
const char* const kTrueStringValue = "1";
const char* const kFalseStringValue = "0";
static const char kArgumentSeparator = '|';

// A helper function to convert a bool to a string, used when assembling
// messages posted back to the browser.  |true| is converted to "1", |false| to
// "0".
const std::string BoolToString(bool bool_value) {
  return bool_value ? kTrueStringValue : kFalseStringValue;
}

PPP_Instance instance_interface;
PPP_Messaging messaging_interface;
Module* singleton_ = NULL;

}  // namespace

PP_Bool Instance_DidCreate(PP_Instance pp_instance,
                           uint32_t argc,
                           const char* argn[],
                           const char* argv[]) {
  printf("--- Instance_DidCreate\n");
  return PP_TRUE;
}

void Instance_DidDestroy(PP_Instance instance) {
  printf("--- Instance_DidDestroy\n");
}

void Instance_DidChangeView(PP_Instance pp_instance, PP_Resource view) {
}

void Instance_DidChangeFocus(PP_Instance pp_instance,
                             PP_Bool has_focus) {
}

PP_Bool Instance_HandleDocumentLoad(PP_Instance pp_instance,
                                    PP_Resource pp_url_loader) {
  return PP_FALSE;
}

void Messaging_HandleMessage(PP_Instance instance, struct PP_Var var_message) {
  if (var_message.type != PP_VARTYPE_STRING)
    return;
  std::string message = Module::Get()->VarToStr(var_message);
  Module::Get()->ppb_var_interface()->Release(var_message);
  printf("--- Messaging_HandleMessage(%s)\n", message.c_str());
  // Look for the "loadWebSocket" message.  The expected string format looks like:
  //     loadUrl|<url>|<stream_as_file>
  // loadUrl is a string literal
  // <url> is the URL used to make the GET request in UrlLoader
  // <stream_as_file> represent Boolean true if it's a '1' or false if it's
  // anything else.
  if (message.find(kLoadUrlMethodId) != 0)
    return;

  size_t url_pos = message.find_first_of(kArgumentSeparator);
  if (url_pos == std::string::npos || url_pos + 1 >= message.length())
    return;

  size_t ping_data_pos = message.find_first_of(kArgumentSeparator, url_pos + 1);
  if (ping_data_pos == std::string::npos || ping_data_pos + 1 >= message.length())
    return;

  size_t url_length = ping_data_pos - url_pos;
  if (url_length == 0)
    return;
  std::string url = message.substr(url_pos + 1, url_length - 1);

  // If the second argument is a '1', assume it means |stream_as_file| is
  // true.  Anything else means |stream_as_file| is false.
  std::string hex_encoded_message = message.substr(ping_data_pos + 1);

  printf("--- Messaging_HandleMessage(method='%s', "
                                      "url='%s', "
                                      "hexdump='%s')\n",
         message.c_str(),
         url.c_str(),
         hex_encoded_message.c_str());
  fflush(stdout);

  WebSocketRequest* web_socket_request = new WebSocketRequest(instance);
  if (NULL == web_socket_request) {
    Module::Get()->ReportResult(instance,
                                url.c_str(),
                                "LoadUrl: memory allocation failed",
                                strlen("LoadUrl: memory allocation failed"),
                                false);
    return;
  }
  // On success or failure web_socket_request will call ReportResult().
  // This is the time to clean it up.
  web_socket_request->set_delete_this_after_report();
  web_socket_request->Load(hex_encoded_message, url);
}

Module* Module::Create(PP_Module module_id,
                       PPB_GetInterface get_browser_interface) {
  if (NULL == singleton_) {
    singleton_ = new Module(module_id, get_browser_interface);
  }
  return singleton_;
}

Module* Module::Get() {
  return singleton_;
}

void Module::Free() {
  delete singleton_;
  singleton_ = NULL;
}

Module::Module(PP_Module module_id, PPB_GetInterface get_browser_interface)
  : module_id_(module_id),
    get_browser_interface_(get_browser_interface),
    ppb_core_interface_(NULL),
    ppb_messaging_interface_(NULL),
    ppb_var_interface_(NULL),
    ppb_var_array_buffer_interface_(NULL) {
  printf("--- Module::Module\n");
  memset(&instance_interface, 0, sizeof(instance_interface));
  instance_interface.DidCreate = Instance_DidCreate;
  instance_interface.DidDestroy = Instance_DidDestroy;
  instance_interface.DidChangeView = Instance_DidChangeView;
  instance_interface.DidChangeFocus = Instance_DidChangeFocus;
  instance_interface.HandleDocumentLoad = Instance_HandleDocumentLoad;

  memset(&messaging_interface, 0, sizeof(messaging_interface));
  messaging_interface.HandleMessage = Messaging_HandleMessage;

  ppb_core_interface_ =
      static_cast<const PPB_Core*>(
          this->GetBrowserInterface(PPB_CORE_INTERFACE));
  ppb_messaging_interface_ =
      static_cast<const PPB_Messaging*>(
          this->GetBrowserInterface(PPB_MESSAGING_INTERFACE));
  ppb_var_interface_ =
      static_cast<const PPB_Var*>(
          this->GetBrowserInterface(PPB_VAR_INTERFACE));
  ppb_var_array_buffer_interface_ =
      static_cast<const PPB_VarArrayBuffer*>(
          this->GetBrowserInterface(PPB_VAR_ARRAY_BUFFER_INTERFACE));
}

const void* Module::GetPluginInterface(const char* interface_name) {
  printf("--- Module::GetPluginInterface(%s)\n", interface_name);
  if (strcmp(interface_name, PPP_INSTANCE_INTERFACE) == 0)
    return &instance_interface;
  if (strcmp(interface_name, PPP_MESSAGING_INTERFACE) == 0)
    return &messaging_interface;
  return NULL;
}

const void* Module::GetBrowserInterface(const char* interface_name) {
  if (NULL == get_browser_interface_)
    return NULL;
  return (*get_browser_interface_)(interface_name);
}

char* Module::VarToCStr(const PP_Var& var) {
  Module* module = Get();
  if (NULL == module)
    return NULL;
  const PPB_Var* ppb_var = module->ppb_var_interface();
  if (NULL == ppb_var)
    return NULL;
  uint32_t len = 0;
  const char* pp_str = ppb_var->VarToUtf8(var, &len);
  if (NULL == pp_str)
    return NULL;
  char* str = static_cast<char*>(malloc(len + 1));
  if (NULL == str)
    return NULL;
  memcpy(str, pp_str, len);
  str[len] = 0;
  return str;
}

std::string Module::VarToStr(const PP_Var& var) {
  std::string str;
  char* cstr = VarToCStr(var);
  if (NULL != cstr) {
    str = cstr;
    free(cstr);
  }
  return str;
}

PP_Var Module::StrToVar(const char* str) {
  Module* module = Get();
  if (NULL == module)
    return PP_MakeUndefined();
  const PPB_Var* ppb_var = module->ppb_var_interface();
  if (NULL != ppb_var)
    return ppb_var->VarFromUtf8(str, strlen(str));
  return PP_MakeUndefined();
}

PP_Var Module::BufferToVar(const char* data, uint32_t length) {
  Module* module = Get();
  if (NULL == module)
    return PP_MakeUndefined();
  const PPB_VarArrayBuffer* ppb_var = module->ppb_var_array_buffer_interface();
  if (NULL != ppb_var) {
    PP_Var retval = ppb_var->Create(length);
    void * mapped = ppb_var->Map(retval);
    memcpy(mapped, data, length);
    ppb_var->Unmap(retval);
    return retval;
  }
  return PP_MakeUndefined();
}

bool Module::CopyBufferVarData(const PP_Var&var, char* data, uint32_t max_length) {
  Module* module = Get();
  if (NULL == module)
    return false;
  const PPB_VarArrayBuffer* ppb_var = module->ppb_var_array_buffer_interface();
  if (NULL != ppb_var) {
    uint32_t length = 0;
    if (ppb_var->ByteLength(var, &length)) {
      char * mapped = reinterpret_cast<char*>(ppb_var->Map(var));
      memcpy(data, mapped,(length > max_length ? max_length : length));
      ppb_var->Unmap(var);
      return max_length >= length;
    } else {
        return false;
    }
  }
  return false;
}

uint32_t Module::BufferVarLength(const PP_Var&var) {
  Module* module = Get();
  if (NULL == module)
    return 0;
  const PPB_VarArrayBuffer* ppb_var = module->ppb_var_array_buffer_interface();
  if (NULL != ppb_var) {
    uint32_t length = 0;
    if (ppb_var->ByteLength(var, &length)) {
      return length;
    }
  }
  return 0;
}

PP_Var Module::StrToVar(const std::string& str) {
  return Module::StrToVar(str.c_str());
}

std::string Module::ErrorCodeToStr(int32_t error_code) {
  switch (error_code) {
    case PP_OK: return "PP_OK";
    case PP_OK_COMPLETIONPENDING: return "PP_OK_COMPLETIONPENDING";
    case PP_ERROR_FAILED: return "PP_ERROR_FAILED";
    case PP_ERROR_ABORTED: return "PP_ERROR_ABORTED";
    case PP_ERROR_BADARGUMENT: return "PP_ERROR_BADARGUMENT";
    case PP_ERROR_BADRESOURCE: return "PP_ERROR_BADRESOURCE";
    case PP_ERROR_NOINTERFACE: return "PP_ERROR_NOINTERFACE";
    case PP_ERROR_NOACCESS: return "PP_ERROR_NOACCESS";
    case PP_ERROR_NOMEMORY: return "PP_ERROR_NOMEMORY";
    case PP_ERROR_NOSPACE: return "PP_ERROR_NOSPACE";
    case PP_ERROR_NOQUOTA: return "PP_ERROR_NOQUOTA";
    case PP_ERROR_INPROGRESS: return "PP_ERROR_INPROGRESS";
    case PP_ERROR_FILENOTFOUND: return "PP_ERROR_FILENOTFOUND";
    case PP_ERROR_FILEEXISTS: return "PP_ERROR_FILEEXISTS";
    case PP_ERROR_FILETOOBIG: return "PP_ERROR_FILETOOBIG";
    case PP_ERROR_FILECHANGED: return "PP_ERROR_FILECHANGED";
    case PP_ERROR_TIMEDOUT: return "PP_ERROR_TIMEDOUT";
  }
  return "N/A";
}

void Module::ReportResult(PP_Instance pp_instance,
                          const char* url,
                          const char * data_to_send,
                          size_t length,
                          bool success) {
  std::string msg(data_to_send, length);
  printf("--- ReportResult('%s', hex_file=%s, success=%d)\n",
         url, msg.c_str(), success);
  // Post a message with the results back to the browser.
  std::string result(url);
  result += kArgumentSeparator;
  result += std::string(data_to_send, length);
  result += kArgumentSeparator;
  result += std::string(data_to_send, length);
  result += kArgumentSeparator;
  result += BoolToString(success);
  printf("--- ReportResult posts result string:\n\t%s\n", result.c_str());
  struct PP_Var var_result = StrToVar(result);
  ppb_messaging_interface()->PostMessage(pp_instance, var_result);
  // If the message was created using VarFromUtf8() it needs to be released.
  // See the comments about VarFromUtf8() in ppapi/c/ppb_var.h for more
  // information.
  if (var_result.type == PP_VARTYPE_STRING) {
    ppb_var_interface()->Release(var_result);
  }
}
