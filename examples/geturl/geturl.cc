// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This example demonstrates how to load content of the page into NaCl module.

#include <cstdio>
#include <string>
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/url_loader.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"

#include "geturl_handler.h"

// These are the method names as JavaScript sees them.
namespace {
const char* const kLoadUrlMethodId = "getUrl";
static const char kMessageArgumentSeparator = ':';

// Exception strings.  These are passed back to the browser when errors
// happen during property accesses or method calls.
const char* const kExceptionStartFailed = "GetURLHandler::Start() failed";
const char* const kExceptionURLNotAString = "URL is not a string";
}  // namespace

// The Instance class.  One of these exists for each instance of your NaCl
// module on the web page.  The browser will ask the Module object to create
// a new Instance for each occurrence of the <embed> tag that has these
// attributes:
//     type="application/x-nacl"
//     src="geturl.nmf"
class GetURLInstance : public pp::Instance {
 public:
  explicit GetURLInstance(PP_Instance instance) : pp::Instance(instance) {}
  virtual ~GetURLInstance() {}

  // Called by the browser to handle the postMessage() call in Javascript.
  // The message in this case is expected to contain the string 'getUrl'
  // followed by a ':' separator, then the URL to fetch.  If a valid message
  // of the form 'getUrl:URL' is received, then start up an asynchronous
  // download of URL.  In the event that errors occur, this method posts an
  // error string back to the browser.
  virtual void HandleMessage(const pp::Var& var_message);
};

void GetURLInstance::HandleMessage(const pp::Var& var_message) {
  if (!var_message.is_string()) {
    return;
  }
  std::string message = var_message.AsString();
  if (message.find(kLoadUrlMethodId) == 0) {
    // The argument to getUrl is everything after the first ':'.
    size_t sep_pos = message.find_first_of(kMessageArgumentSeparator);
    if (sep_pos != std::string::npos) {
      std::string url = message.substr(sep_pos + 1);
      printf("GetURLInstance::HandleMessage('%s', '%s')\n",
             message.c_str(),
             url.c_str());
      fflush(stdout);
      GetURLHandler* handler = GetURLHandler::Create(this, url);
      if (handler != NULL) {
        // Starts asynchronous download. When download is finished or when an
        // error occurs, |handler| posts the results back to the browser
        // vis PostMessage and self-destroys.
        handler->Start();
      }
    }
  }
}


// The Module class.  The browser calls the CreateInstance() method to create
// an instance of you NaCl module on the web page.  The browser creates a new
// instance for each <embed> tag with type="application/x-nacl".
class GetURLModule : public pp::Module {
 public:
  GetURLModule() : pp::Module() {}
  virtual ~GetURLModule() {}

  // Create and return a GetURLInstance object.
  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new GetURLInstance(instance);
  }
};

// Factory function called by the browser when the module is first loaded.
// The browser keeps a singleton of this module.  It calls the
// CreateInstance() method on the object you return to make instances.  There
// is one instance per <embed> tag on the page.  This is the main binding
// point for your NaCl module with the browser.
namespace pp {
Module* CreateModule() {
  return new GetURLModule();
}
}  // namespace pp

