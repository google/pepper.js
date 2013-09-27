// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tests/ppapi_geturl/url_load_request.h"

#include <stdio.h>
#include <string>
#include <sstream>

//#include "native_client/src/include/portability.h"
//#include "native_client/src/include/nacl_macros.h"
//#include "native_client/src/shared/platform/nacl_check.h"
#include "tests/common.h"
#include "tests/ppapi_geturl/module.h"
#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"

namespace {

// A local helper that does not contribute to loading/reading of urls, but
// allows us to test proxying of Is<Interface> functions.
// TODO(polina): when we have unit tests, move this there.
void TestIsInterface(std::string test_interface,
                     PP_Resource resource,
                     const PPB_FileIO* ppb_fileio,
                     const PPB_URLRequestInfo* ppb_url_request_info,
                     const PPB_URLResponseInfo* ppb_url_response_info,
                     const PPB_URLLoader* ppb_url_loader) {
  printf("--- TestIsInterface: %s\n", test_interface.c_str());
  if (test_interface == PPB_FILEIO_INTERFACE) {
    CHECK(ppb_fileio->IsFileIO(resource) == PP_TRUE);
    CHECK(ppb_url_request_info->IsURLRequestInfo(resource) == PP_FALSE);
    CHECK(ppb_url_response_info->IsURLResponseInfo(resource) == PP_FALSE);
    CHECK(ppb_url_loader->IsURLLoader(resource) == PP_FALSE);
  } else if (test_interface == PPB_URLREQUESTINFO_INTERFACE) {
    CHECK(ppb_fileio->IsFileIO(resource) == PP_FALSE);
    CHECK(ppb_url_request_info->IsURLRequestInfo(resource) == PP_TRUE);
    CHECK(ppb_url_response_info->IsURLResponseInfo(resource) == PP_FALSE);
    CHECK(ppb_url_loader->IsURLLoader(resource) == PP_FALSE);
  } else if (test_interface == PPB_URLRESPONSEINFO_INTERFACE) {
    CHECK(ppb_fileio->IsFileIO(resource) == PP_FALSE);
    CHECK(ppb_url_request_info->IsURLRequestInfo(resource) == PP_FALSE);
    CHECK(ppb_url_response_info->IsURLResponseInfo(resource) == PP_TRUE);
    CHECK(ppb_url_loader->IsURLLoader(resource) == PP_FALSE);
  } else if (test_interface == PPB_URLLOADER_INTERFACE) {
    CHECK(ppb_fileio->IsFileIO(resource) == PP_FALSE);
    CHECK(ppb_url_request_info->IsURLRequestInfo(resource) == PP_FALSE);
    CHECK(ppb_url_response_info->IsURLResponseInfo(resource) == PP_FALSE);
    CHECK(ppb_url_loader->IsURLLoader(resource) == PP_TRUE);
  }
}

void OpenCallback(void* user_data, int32_t pp_error) {
  UrlLoadRequest* obj = reinterpret_cast<UrlLoadRequest*>(user_data);
  if (NULL != obj)
    obj->OpenCallback(pp_error);
}

void FinishStreamingToFileCallback(void* user_data, int32_t pp_error) {
  UrlLoadRequest* obj = reinterpret_cast<UrlLoadRequest*>(user_data);
  if (NULL != obj)
    obj->FinishStreamingToFileCallback(pp_error);
}

void ReadResponseBodyCallback(void* user_data, int32_t pp_error_or_bytes) {
  UrlLoadRequest* obj = reinterpret_cast<UrlLoadRequest*>(user_data);
  if (NULL != obj)
    obj->ReadResponseBodyCallback(pp_error_or_bytes);
}

void OpenFileBodyCallback(void* user_data, int32_t pp_error) {
  UrlLoadRequest* obj = reinterpret_cast<UrlLoadRequest*>(user_data);
  if (NULL != obj)
    obj->OpenFileBodyCallback(pp_error);
}

void ReadFileBodyCallback(void* user_data, int32_t pp_error_or_bytes) {
  UrlLoadRequest* obj = reinterpret_cast<UrlLoadRequest*>(user_data);
  if (NULL != obj)
    obj->ReadFileBodyCallback(pp_error_or_bytes);
}

}  // namespace


UrlLoadRequest::UrlLoadRequest(PP_Instance instance)
    : delete_this_after_report(false),
      as_file_(false),
      instance_(instance),
      request_(0),
      loader_(0),
      response_(0),
      fileio_(0),
      request_interface_(NULL),
      loader_interface_(NULL),
      fileio_interface_(NULL),
      read_offset_(0) {
}

UrlLoadRequest::~UrlLoadRequest() {
  Clear();
}

void UrlLoadRequest::Clear() {
  Module* module = Module::Get();
  if (0 != request_) {
    module->ppb_core_interface()->ReleaseResource(request_);
    request_ = 0;
  }
  if (0 != loader_) {
    module->ppb_core_interface()->ReleaseResource(loader_);
    loader_ = 0;
  }
  if (0 != response_) {
    module->ppb_core_interface()->ReleaseResource(response_);
    response_ = 0;
  }
  if (0 != fileio_) {
    module->ppb_core_interface()->ReleaseResource(fileio_);
    fileio_ = 0;
  }
  url_body_.clear();
}

bool UrlLoadRequest::ReportSuccess() {
  Module::Get()->ReportResult(
      instance_, url_.c_str(), as_file_, url_body_.c_str(), true);
  if (delete_this_after_report) {
    delete this;
  }
  return true;
}

bool UrlLoadRequest::ReportFailure(const std::string& error) {
  Module::Get()->ReportResult(
      instance_, url_.c_str(), as_file_, error.c_str(), false);
  if (delete_this_after_report) {
    delete this;
  }
  return false;
}

bool UrlLoadRequest::ReportFailure(const std::string& message,
                                   int32_t pp_error) {
  std::string error = message;
  error.append(Module::ErrorCodeToStr(pp_error));
  return ReportFailure(error);
}

bool UrlLoadRequest::Load(bool as_file, std::string url) {
  printf("--- UrlLoadRequest::Load(as_file=%d, '%s')\n", as_file, url.c_str());
  url_ = url;
  as_file_ = as_file;
  Clear();
  std::string error;
  if (!GetRequiredInterfaces(&error)) {
    return ReportFailure(error);
  }
  PP_Var value = Module::StrToVar(url);
  PP_Bool set_url = request_interface_->SetProperty(
      request_, PP_URLREQUESTPROPERTY_URL, value);
  Module::Get()->ppb_var_interface()->Release(value);

  value = Module::StrToVar("GET");
  PP_Bool set_method = request_interface_->SetProperty(
      request_, PP_URLREQUESTPROPERTY_METHOD, value);
  Module::Get()->ppb_var_interface()->Release(value);

  PP_Bool pp_as_file = as_file ? PP_TRUE : PP_FALSE;
  PP_Bool set_file = request_interface_->SetProperty(
      request_, PP_URLREQUESTPROPERTY_STREAMTOFILE, PP_MakeBool(pp_as_file));
  if (set_url != PP_TRUE || set_method != PP_TRUE || set_file != PP_TRUE) {
    return ReportFailure("PPB_URLRequestInfo::SetProperty: failed");
  }
  loader_interface_->Open(
      loader_,
      request_,
      PP_MakeCompletionCallback(::OpenCallback, this));
  return true;
}

bool UrlLoadRequest::GetRequiredInterfaces(std::string* error) {
  Module* module = Module::Get();

  request_interface_ = static_cast<const PPB_URLRequestInfo*>(
      module->GetBrowserInterface(PPB_URLREQUESTINFO_INTERFACE));
  if (NULL == request_interface_) {
    *error = "Failed to get browser interface '" PPB_URLREQUESTINFO_INTERFACE;
    return false;
  }
  request_ = request_interface_->Create(instance_);
  if (0 == request_) {
    *error = "PPB_URLRequestInfo::Create: failed";
    return false;
  }

  response_interface_ = static_cast<const PPB_URLResponseInfo*>(
      module->GetBrowserInterface(PPB_URLRESPONSEINFO_INTERFACE));
  if (NULL == response_interface_) {
    *error = "Failed to get browser interface '" PPB_URLRESPONSEINFO_INTERFACE;
    return false;
  }

  loader_interface_ = static_cast<const PPB_URLLoader*>(
          module->GetBrowserInterface(PPB_URLLOADER_INTERFACE));
  if (NULL == loader_interface_) {
    *error = "Failed to get browser interface '" PPB_URLLOADER_INTERFACE;
    return false;
  }
  loader_ = loader_interface_->Create(instance_);
  if (0 == loader_) {
    *error = "PPB_URLLoader::Create: failed";
    return false;
  }

  fileio_interface_ = static_cast<const PPB_FileIO*>(
      module->GetBrowserInterface(PPB_FILEIO_INTERFACE));
  if (NULL == fileio_interface_) {
    *error = "Failed to get browser interface '" PPB_FILEIO_INTERFACE;
    return false;
  }
  fileio_ = fileio_interface_->Create(instance_);
  if (0 == fileio_) {
    *error = "PPB_FileIO::Create: failed";
    return false;
  }

  TestIsInterface(PPB_URLREQUESTINFO_INTERFACE, request_,
                  fileio_interface_, request_interface_, response_interface_,
                  loader_interface_);
  TestIsInterface(PPB_URLLOADER_INTERFACE, loader_,
                  fileio_interface_, request_interface_, response_interface_,
                  loader_interface_);
  TestIsInterface(PPB_FILEIO_INTERFACE, fileio_,
                  fileio_interface_, request_interface_, response_interface_,
                  loader_interface_);

  return true;
}

void UrlLoadRequest::ReadResponseBody() {
  loader_interface_->ReadResponseBody(
      loader_,
      buffer_,
      sizeof(buffer_),
      PP_MakeCompletionCallback(::ReadResponseBodyCallback, this));
}

void UrlLoadRequest::ReadFileBody() {
  fileio_interface_->Read(
      fileio_,
      read_offset_,
      buffer_,
      sizeof(buffer_),
      PP_MakeCompletionCallback(::ReadFileBodyCallback, this));
}

void UrlLoadRequest::OpenCallback(int32_t pp_error) {
  printf("--- UrlLoadRequest::OpenCallback\n");
  if (pp_error != PP_OK) {
    ReportFailure("UrlLoadRequest::OpenCallback: ", pp_error);
    return;
  }

  // Validating response headers to confirm successful loading.
  response_ = loader_interface_->GetResponseInfo(loader_);
  if (0 == response_) {
    ReportFailure("UrlLoadRequest::OpenCallback: null response");
    return;
  }
  TestIsInterface(PPB_URLRESPONSEINFO_INTERFACE, response_,
                  fileio_interface_, request_interface_, response_interface_,
                  loader_interface_);
  PP_Var url = response_interface_->GetProperty(response_,
                                                PP_URLRESPONSEPROPERTY_URL);
  if (url.type != PP_VARTYPE_STRING) {
    ReportFailure("URLLoadRequest::OpenCallback: bad url type");
    return;
  }
  url_ = Module::VarToStr(url);  // Update url to be fully qualified.
  Module::Get()->ppb_var_interface()->Release(url);

  PP_Var status_code =
      response_interface_->GetProperty(response_,
                                       PP_URLRESPONSEPROPERTY_STATUSCODE);
  int32_t status_code_as_int = status_code.value.as_int;
  if (status_code_as_int != 200) {  // Not HTTP OK.
    std::stringstream error;
    error << "OpenCallback: status_code=" << status_code_as_int;
    ReportFailure(error.str());
    return;
  }

  if (as_file_) {
    loader_interface_->FinishStreamingToFile(
        loader_,
        PP_MakeCompletionCallback(::FinishStreamingToFileCallback, this));
  } else {
    ReadResponseBody();
  }
}

void UrlLoadRequest::FinishStreamingToFileCallback(int32_t pp_error) {
  printf("--- UrlLoadRequest::FinishStreamingToFileCallback\n");
  if (pp_error != PP_OK) {
    ReportFailure("UrlLoadRequest::FinishStreamingToFileCallback: ", pp_error);
    return;
  }
  PP_Resource fileref = response_interface_->GetBodyAsFileRef(response_);
  if (0 == fileref) {
    ReportFailure("UrlLoadRequest::FinishStreamingToFileCallback: null file");
    return;
  }
  fileio_interface_->Open(
      fileio_,
      fileref,
      PP_FILEOPENFLAG_READ,
      PP_MakeCompletionCallback(::OpenFileBodyCallback, this));
}

void UrlLoadRequest::ReadResponseBodyCallback(int32_t pp_error_or_bytes) {
  printf("--- UrlLoadRequest::ReadResponseBodyCallback\n");
  if (pp_error_or_bytes < PP_OK) {
    ReportFailure("UrlLoadRequest::ReadResponseBodyCallback: ",
                  pp_error_or_bytes);
  } else if (pp_error_or_bytes == PP_OK) {  // Reached EOF.
    ReportSuccess();
  } else {  // Partial read, so copy out the buffer and continue reading.
    for (int32_t i = 0; i < pp_error_or_bytes; i++)
      url_body_.push_back(buffer_[i]);
    ReadResponseBody();
  }
}

void UrlLoadRequest::ReadFileBodyCallback(int32_t pp_error_or_bytes) {
  printf("--- UrlLoadRequest::ReadFileBodyCallback\n");
  if (pp_error_or_bytes < PP_OK) {
    ReportFailure("UrlLoadRequest::ReadFileBodyCallback: ",
                  pp_error_or_bytes);
  } else if (pp_error_or_bytes == PP_OK) {  // Reached EOF.
    ReportSuccess();
  } else {  // Partial read, so copy out the buffer and continue reading.
    for (int32_t i = 0; i < pp_error_or_bytes; i++)
      url_body_.push_back(buffer_[i]);
    read_offset_ += pp_error_or_bytes;
    ReadFileBody();
  }
}

void UrlLoadRequest::OpenFileBodyCallback(int32_t pp_error) {
  printf("--- UrlLoadRequest::OpenFileBodyCallback\n");
  if (pp_error != PP_OK) {
    ReportFailure("UrlLoadRequest::OpenFileBodyCallback: ", pp_error);
    return;
  }
  ReadFileBody();
}
