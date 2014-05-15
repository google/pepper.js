// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tests/ppapi_websocket/web_socket_request.h"

#include <stdio.h>
#include <string>
#include <sstream>

#include "tests/common.h"
#include "tests/ppapi_websocket/module.h"
#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"

namespace {

// A local helper that does not contribute to loading/reading of urls, but
// allows us to test proxying of Is<Interface> functions.
// TODO(polina): when we have unit tests, move this there.
void TestIsInterface(std::string test_interface,
                     PP_Resource resource,
                     const PPB_WebSocket* ppb_web_socket) {
  printf("--- TestIsInterface: %s\n", test_interface.c_str());
  if (test_interface == PPB_WEBSOCKET_INTERFACE) {
    CHECK(ppb_web_socket->IsWebSocket(resource) == PP_TRUE);
  }
}

void OpenCallback(void* user_data, int32_t pp_error) {
  WebSocketRequest* obj = reinterpret_cast<WebSocketRequest*>(user_data);
  if (NULL != obj)
    obj->OpenCallback(pp_error);
}

void ReadResponse(void* user_data, int32_t pp_error_or_bytes) {
  WebSocketRequest* obj = reinterpret_cast<WebSocketRequest*>(user_data);
  if (NULL != obj)
    obj->ReadResponse(pp_error_or_bytes);
}

void CloseResponse(void* user_data, int32_t pp_error_or_bytes) {
  WebSocketRequest* obj = reinterpret_cast<WebSocketRequest*>(user_data);
  if (NULL != obj)
    obj->CloseResponse(pp_error_or_bytes);
}

}  // namespace


WebSocketRequest::WebSocketRequest(PP_Instance instance)
    : delete_this_after_report(false),
      instance_(instance),
      websocket_(0),
      websocket_interface_(NULL) {
}

WebSocketRequest::~WebSocketRequest() {
  Clear();
}

void WebSocketRequest::Clear() {
  Module* module = Module::Get();
  if (0 != websocket_) {
    module->ppb_core_interface()->ReleaseResource(websocket_);
    websocket_ = 0;
  }
}

bool WebSocketRequest::ReportSuccess() {
  Module::Get()->ReportResult(
      instance_, url_.c_str(), hex_file.data(), hex_file.length(), true);
  if (delete_this_after_report) {
    delete this;
  }
  return true;
}

bool WebSocketRequest::ReportFailure(const std::string& error) {
  Module::Get()->ReportResult(
      instance_, url_.c_str(), error.c_str(), error.length(), false);
  if (delete_this_after_report) {
    delete this;
  }
  return false;
}

bool WebSocketRequest::ReportFailure(const std::string& message,
                                   int32_t pp_error) {
  std::string error = message;
  error.append(Module::ErrorCodeToStr(pp_error));
  return ReportFailure(error);
}

bool WebSocketRequest::Load(std::string hex_file, std::string url) {
  printf("--- WebSocketRequest::Load('%s') <- %s\n", url.c_str(), hex_file.c_str());
  url_ = url;
  Clear();
  std::string error;
  if (!GetRequiredInterfaces(&error)) {
    return ReportFailure(error);
  }
  PP_Var value = Module::StrToVar(url);
  PP_Var protocol = Module::StrToVar("draft-ietf-hybi-17");
  this->hex_file = hex_file;
  websocket_interface_->Connect(
      websocket_,
      value,
      &protocol,
      0, // no protocol specified
      PP_MakeCompletionCallback(::OpenCallback, this));
  Module::Get()->ppb_var_interface()->Release(protocol);
  Module::Get()->ppb_var_interface()->Release(value);
  PP_Var returned_url = websocket_interface_->GetURL(websocket_);
  std::string returned_url_ = Module::VarToStr(returned_url);

  if (returned_url_ != url) {
      ReportFailure("Returned url " + returned_url_ + " != "+url);
  }
  Module::Get()->ppb_var_interface()->Release(returned_url);
  return true;
}

bool WebSocketRequest::GetRequiredInterfaces(std::string* error) {
  Module* module = Module::Get();

  websocket_interface_ = static_cast<const PPB_WebSocket*>(
          module->GetBrowserInterface(PPB_WEBSOCKET_INTERFACE));
  if (NULL == websocket_interface_) {
    *error = "Failed to get browser interface '" PPB_WEBSOCKET_INTERFACE;
    return false;
  }
  websocket_ = websocket_interface_->Create(instance_);
  if (0 == websocket_) {
    *error = "PPB_URLLoader::Create: failed";
    return false;
  }

  TestIsInterface(PPB_WEBSOCKET_INTERFACE, websocket_,
                  websocket_interface_);
  return true;
}

unsigned char hexNibbleToChar(char nibble) {
    if (nibble <= '9' && nibble >= '0') {
        return nibble - '0';
    }
    if (nibble >= 'A' && nibble <= 'F') {
        return nibble - 'A' + 10;
    }
    return nibble - 'a' + 10;
}
unsigned char hexToByte(char msn, char lsn) {
    return hexNibbleToChar(msn) * 16 + hexNibbleToChar(lsn);
}
void WebSocketRequest::CloseResponse(int32_t error) {
    printf("--- WebSocketRequest::CloseResponse\n");
    if (error != PP_OK) {
        ReportFailure("Unable to close socket", error);
        return;
    }
    ReportSuccess();
}
unsigned char nibbleToHex(unsigned char nibble) {
    if (nibble >= 10) {
        return 'a' + nibble - 10;
    }
    return '0' + nibble;
}
void dataToHex(unsigned char * input, char * hex, uint32_t length) {
    for (uint32_t i=0;i<length;++i) {
        hex[i*2] = nibbleToHex(input[i] / 16);
        hex[i*2 + 1] = nibbleToHex(input[i] & 15);
    }
}
void WebSocketRequest::ReadResponse(int32_t error) {
    printf("--- WebSocketRequest::ReadResponse\n");
    Module* module = Module::Get();
    uint32_t length = module->BufferVarLength(response_array);
    if (length > 0 && length * 2 == hex_file.length()) {
        unsigned char * data = new unsigned char[length];
        char * hexData = new char[length * 2 + 1];
        hexData[length * 2] = '\0';
        if (module->CopyBufferVarData(response_array, reinterpret_cast<char*>(data), length) ){
            dataToHex(data, hexData, length);
            std::string outputHexData(hexData, length * 2);
            if (outputHexData != hex_file) {
                std::string error_message("Bytes Mismatch Output:[");
                error_message += outputHexData;
                error_message += "] Input:[";
                error_message += hex_file;
                error_message += "]";
                ReportFailure(error_message.c_str());
            }
            Module::Get()->ppb_var_interface()->Release(response_array);
            PP_Var close_reason = Module::StrToVar("Done");
            int32_t retcode = websocket_interface_->Close(
                websocket_,
                PP_WEBSOCKETSTATUSCODE_NORMAL_CLOSURE,
                close_reason,
                PP_MakeCompletionCallback(::CloseResponse, this));
            Module::Get()->ppb_var_interface()->Release(close_reason);
        } else {
            ReportFailure("Unable to copy bytes of wrong length");
        }
        delete [] data;
        delete [] hexData;
    } else {
        ReportFailure("Size Mismatch");
    }
}
void WebSocketRequest::OpenCallback(int32_t pp_error) {
  printf("--- WebSocketRequest::OpenCallback\n");
  Module* module = Module::Get();
  if (pp_error != PP_OK) {
    ReportFailure("WebSocketRequest::OpenCallback: ", pp_error);
    return;
  }
  uint32_t len = hex_file.length()/2;
  char * data_to_send = new char[len];
  for (uint32_t i=0; i<len; ++i) {
      data_to_send[i] = hexToByte(hex_file[i * 2], hex_file[i * 2 + 1]);
  }
  send_array = module->BufferToVar(data_to_send, len);
  int32_t retcode = websocket_interface_->SendMessage(
      websocket_,
      send_array);
  Module::Get()->ppb_var_interface()->Release(send_array);
  if (retcode != PP_OK) {
    ReportFailure("WebSocketRequest::SendMessage: ", retcode);
  }
  response_array = PP_MakeUndefined();
  retcode = websocket_interface_->ReceiveMessage(
      websocket_,
      &response_array,
      PP_MakeCompletionCallback(::ReadResponse, this));
  if (retcode == PP_OK) {
      this->ReadResponse(PP_OK);
  } else if (retcode != PP_OK_COMPLETIONPENDING) {
    ReportFailure("WebSocketRequest::ReceiveMessage: ", retcode);
  }
  PP_Var returned_protocol = websocket_interface_->GetProtocol(websocket_);
  PP_Var returned_extensions = websocket_interface_->GetExtensions(websocket_);
  std::string returned_protocol_ = Module::VarToStr(returned_protocol);
  std::string returned_extensions_ = Module::VarToStr(returned_extensions);
  if (returned_protocol_ != "") {
      ReportFailure("Returned protocol " + returned_protocol_ + " != empty");
  }
  if (returned_extensions_ != "") {
      ReportFailure("Returned extensions " + returned_extensions_ + " != empty");
  }
  Module::Get()->ppb_var_interface()->Release(returned_extensions);
  Module::Get()->ppb_var_interface()->Release(returned_protocol);
}
