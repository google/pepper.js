// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef TESTS_PPAPI_GETURL_URL_LOAD_REQUEST_H_
#define TESTS_PPAPI_GETURL_URL_LOAD_REQUEST_H_

#include <string>
#include <vector>

#include "ppapi/c/ppb_file_io.h"
#include "ppapi/c/ppb_websocket.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"


class WebSocketRequest {
 public:
  explicit WebSocketRequest(PP_Instance instance);
  ~WebSocketRequest();
  bool Load(std::string hex_data, std::string url);

  void OpenCallback(int32_t pp_error);
  // Loading/reading via response includes the following asynchronous steps:
  // 1) URLLoader::Open
  // 2) URLLoader::ReadResponseBody (in a loop until EOF)
  void ReadResponse(int32_t pp_error_or_bytes);
  void CloseResponse(int32_t pp_error_or_bytes);
  // Loading/reading via file includes the following asynchronous steps:
  // 1) URLLoader::Open
  // 2) URLLoader::FinishStreamingToFile
  // 3) FileIO::Open
  // 4) FileIO::Read (in a loop until EOF)
  void FinishStreamingToFileCallback(int32_t pp_error);
  void OpenFileBodyCallback(int32_t pp_error);
  void ReadFileBodyCallback(int32_t pp_error_or_bytes);

  void set_delete_this_after_report() { delete_this_after_report = true; }

 private:
  bool GetRequiredInterfaces(std::string* error);
  void Clear();

  void ReadResponseBody();
  void ReadFileBody();

  bool ReportSuccess();
  bool ReportFailure(const std::string& error);
  bool ReportFailure(const std::string& message, int32_t pp_error);

  bool delete_this_after_report;
  std::string hex_file; // hex encoded characters to send over socket
  std::string url_;

  PP_Instance instance_;
  PP_Resource websocket_;
  PP_Var send_array;
  PP_Var response_array;
  const PPB_WebSocket* websocket_interface_;

  char buffer_[1024];
  std::string url_body_;
  int32_t read_offset_;
};

#endif  // TESTS_PPAPI_GETURL_URL_LOAD_REQUEST_H_
