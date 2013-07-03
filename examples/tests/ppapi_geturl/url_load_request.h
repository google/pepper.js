// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef TESTS_PPAPI_GETURL_URL_LOAD_REQUEST_H_
#define TESTS_PPAPI_GETURL_URL_LOAD_REQUEST_H_

#include <string>
#include <vector>

#include "ppapi/c/ppb_file_io.h"
#include "ppapi/c/ppb_file_ref.h"
#include "ppapi/c/ppb_url_loader.h"
#include "ppapi/c/ppb_url_request_info.h"
#include "ppapi/c/ppb_url_response_info.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"
//#include "ppapi/c/trusted/ppb_file_io_trusted.h"


class UrlLoadRequest {
 public:
  explicit UrlLoadRequest(PP_Instance instance);
  ~UrlLoadRequest();
  bool Load(bool stream_as_file, std::string url);

  void OpenCallback(int32_t pp_error);
  // Loading/reading via response includes the following asynchronous steps:
  // 1) URLLoader::Open
  // 2) URLLoader::ReadResponseBody (in a loop until EOF)
  void ReadResponseBodyCallback(int32_t pp_error_or_bytes);
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

  std::string url_;
  bool as_file_;

  PP_Instance instance_;
  PP_Resource request_;
  PP_Resource loader_;
  PP_Resource response_;
  PP_Resource fileio_;

  const PPB_URLRequestInfo* request_interface_;
  const PPB_URLResponseInfo* response_interface_;
  const PPB_URLLoader* loader_interface_;
  const PPB_FileIO* fileio_interface_;

  char buffer_[1024];
  std::string url_body_;
  int32_t read_offset_;
};

#endif  // TESTS_PPAPI_GETURL_URL_LOAD_REQUEST_H_
