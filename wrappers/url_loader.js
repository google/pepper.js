(function() {
  var URL_LOADER_RESOURCE = "url_loader";
  var URL_REQUEST_INFO_RESOURCE = "url_request_info";

  var updatePendingRead = function(loader) {
    if (loader.pendingReadCallback) {
      var full_read_possible = loader.data.byteLength >= loader.index + loader.pendingReadSize
      if (loader.done || full_read_possible){
	var cb = loader.pendingReadCallback;
	loader.pendingReadCallback = null;
	var readSize = full_read_possible ? loader.pendingReadSize : loader.data.byteLength - loader.index;
	var index = loader.index;
	loader.index += readSize;
	cb(readSize, new Uint8Array(loader.data, index, readSize));
      }
    }
  }

  var URLLoader_Create = function(instance) {
    return resources.register(URL_LOADER_RESOURCE, {});
  };

  var URLLoader_IsURLLoader = function(resource) {
    return resources.is(resource, URL_LOADER_RESOURCE);
  };

  var URLLoader_Open = function(loader, request, callback) {
    loader = resources.resolve(loader, URL_LOADER_RESOURCE);
    if (loader === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    request = resources.resolve(request, URL_REQUEST_INFO_RESOURCE);
    if (request === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    // TODO(ncbray): what to do if no URL is specified?
    callback = ppapi_glue.convertCompletionCallback(callback);

    var req = new XMLHttpRequest();

    var did_callback = false;

    // Assumed: onprogress will be called before onreadystatechange.
    req.onprogress = function(evt) {
      loader.progress_bytes = evt.loaded;
      loader.progress_total = evt.total;
      if (!did_callback) {
	// Apps may attempt to synchonously GetDownloadProcess inside the callback,
	// so wait until we have progress to report.
	callback(ppapi.PP_OK);
	did_callback = true;
      }
    }
    req.onreadystatechange = function() {
      if (this.readyState == 1) {
      } else if (this.readyState == 2) {
	if (this.status != 200) {
          if (did_callback) {
	    throw "Internal error: got progress event for a bad URL request.";
	  }
	  callback(ppapi.PP_ERROR_FAILED);
	  // TODO(ncbray): prevent any further responses.
          did_callback = true;
	}
      } else if (this.readyState == 3) {
	// Array buffers do not do partial downloads.
      } else {
	loader.data = this.response;
	loader.done = true;
	updatePendingRead(loader);
      }
    };
    req.open(request.method || "GET", request.url);
    req.responseType = "arraybuffer";
    req.send();
    loader.data = '';
    loader.index = 0;
    loader.done = false;
    loader.pendingReadCallback = null;
    loader.pendingReadSize = 0;
    loader.progress_bytes = 0;
    loader.progress_total = -1;

    return ppapi.PP_OK_COMPLETIONPENDING;
  };

  var URLLoader_FollowRedirect = function() {
    throw "URLLoader_FollowRedirect not implemented";
  };
  var URLLoader_GetUploadProgress = function() {
    throw "URLLoader_GetUploadProgress not implemented";
  };

  var URLLoader_GetDownloadProgress = function(loader, bytes_ptr, total_ptr) {
    var l = resources.resolve(loader, URL_LOADER_RESOURCE);
    if (l === undefined) {
      return 0;
    }
    setValue(bytes_ptr, l.progress_bytes, 'i64');
    setValue(total_ptr, l.progress_total, 'i64');
    return 1;
  };

  var URLLoader_GetResponseInfo = function() {
    throw "URLLoader_GetResponseInfo not implemented";
  };

  var URLLoader_ReadResponseBody = function(loader, buffer_ptr, read_size, callback) {
    var loader = resources.resolve(loader, URL_LOADER_RESOURCE);
    if (loader === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    var c = ppapi_glue.convertCompletionCallback(callback);

    loader.pendingReadCallback = function(status, data) {
      HEAP8.set(data, buffer_ptr);
      c(status);
    };
    loader.pendingReadSize = read_size;
    setTimeout(function() { updatePendingRead(loader); }, 0);
    return ppapi.PP_OK_COMPLETIONPENDING;

  };

  var URLLoader_FinishStreamingToFile = function() {
    throw "URLLoader_FinishStreamingToFile not implemented";
  };
  var URLLoader_Close = function() {
    throw "URLLoader_Close not implemented";
  };

  registerInterface("PPB_URLLoader;1.0", [
    URLLoader_Create,
    URLLoader_IsURLLoader,
    URLLoader_Open,
    URLLoader_FollowRedirect,
    URLLoader_GetUploadProgress,
    URLLoader_GetDownloadProgress,
    URLLoader_GetResponseInfo,
    URLLoader_ReadResponseBody,
    URLLoader_FinishStreamingToFile,
    URLLoader_Close
  ]);


  var URLRequestInfo_Create = function(instance) {
    if (resources.resolve(instance, INSTANCE_RESOURCE) === undefined) {
      return 0;
    }
    return resources.register(URL_REQUEST_INFO_RESOURCE, {
      method: "GET",
      headers: "",
      stream_to_file: false,
      follow_redirects: true,
      record_download_progress: false,
      record_upload_progress: false,
      allow_cross_origin_requests: false,
      allow_credentials: false,
      body: null
    });
  };

  var URLRequestInfo_IsURLRequestInfo = function(resource) {
    return resources.is(resource, URL_REQUEST_INFO_RESOURCE);
  };

  var URLRequestInfo_SetProperty = function(request, property, value) {
    var r = resources.resolve(request, URL_REQUEST_INFO_RESOURCE);
    if (r === undefined) {
      return 0;
    }

    // Need to use the ppapi var type to distinguish between ints and floats.
    var var_type = ppapi_glue.varType(value);
    var js_obj = ppapi_glue.jsForVar(value);

    if (property === 0) {
      if (var_type !== ppapi_glue.PP_VARTYPE_STRING) {
        return 0;
      }
      r.url = js_obj;
    } else if (property === 1) {
      if (var_type !== ppapi_glue.PP_VARTYPE_STRING) {
        return 0;
      }
      // PPAPI does not filter invalid methods at this level.
      r.method = js_obj;
    } else if (property === 2) {
      if (var_type !== ppapi_glue.PP_VARTYPE_STRING) {
        return 0;
      }
      r.headers = js_obj;
    } else if (property === 3) {
      if (var_type !== ppapi_glue.PP_VARTYPE_BOOL) {
        return 0;
      }
      r.stream_to_file = js_obj;
    } else if (property === 4) {
      if (var_type !== ppapi_glue.PP_VARTYPE_BOOL) {
        return 0;
      }
      r.follow_redirects = js_obj;
    } else if (property === 5) {
      if (var_type !== ppapi_glue.PP_VARTYPE_BOOL) {
        return 0;
      }
      r.record_download_progress = js_obj;
    } else if (property === 6) {
      if (var_type !== ppapi_glue.PP_VARTYPE_BOOL) {
        return 0;
      }
      r.record_upload_progress = js_obj;
    } else if (property === 7) {
      if (var_type !== ppapi_glue.PP_VARTYPE_STRING && var_type !== ppapi_glue.PP_VARTYPE_UNDEFINED) {
        return 0;
      }
      r.custom_referrer_url = js_obj;
    } else if (property === 8) {
      if (var_type !== ppapi_glue.PP_VARTYPE_BOOL) {
        return 0;
      }
      r.allow_cross_origin_requests = js_obj;
    } else if (property === 9) {
      if (var_type !== ppapi_glue.PP_VARTYPE_BOOL) {
        return 0;
      }
      r.allow_credentials = js_obj;
    } else if (property === 10) {
      if (var_type !== ppapi_glue.PP_VARTYPE_STRING && var_type !== ppapi_glue.PP_VARTYPE_UNDEFINED) {
        return 0;
      }
      r.custom_content_transfer_encoding = js_obj;
    } else if (property === 11) {
      // TODO(ncbray): require integer, disallow double.
      if (var_type !== ppapi_glue.PP_VARTYPE_INT32) {
        return 0;
      }
      r.prefetch_buffer_upper_threshold = js_obj;
    } else if (property === 12) {
      // TODO(ncbray): require integer, disallow double.
      if (var_type !== ppapi_glue.PP_VARTYPE_INT32) {
        return 0;
      }
      r.prefetch_buffer_lower_threshold = js_obj;
    } else if (property === 13) {
      if (var_type !== ppapi_glue.PP_VARTYPE_STRING && var_type !== ppapi_glue.PP_VARTYPE_UNDEFINED) {
        return 0;
      }
      r.custom_user_agent = js_obj;
    } else {
      console.error("URLRequestInfo_SetProperty got unknown property " + property);
      return 0;
    }
    return 1;
  };

  var URLRequestInfo_AppendDataToBody = function(request, data, len) {
    var r = resources.resolve(request, URL_REQUEST_INFO_RESOURCE);
    if (r === undefined) {
      return 0;
    }
    // TODO(ncbray): actually copy and send the data.  Note the data may not be UTF8.
    return 1;
  };

  var URLRequestInfo_AppendFileToBody = function(request, file_ref, start_offset, number_of_bytes, expect_last_time_modified) {
    throw "URLRequestInfo_AppendFileToBody not implemented";
  };

  registerInterface("PPB_URLRequestInfo;1.0", [
    URLRequestInfo_Create,
    URLRequestInfo_IsURLRequestInfo,
    URLRequestInfo_SetProperty,
    URLRequestInfo_AppendDataToBody,
    URLRequestInfo_AppendFileToBody
  ]);


  var URLResponseInfo_IsURLResponseInfo = function(res) {
    throw "URLResponseInfo_IsURLResponseInfo not implemented";
  };

  var URLResponseInfo_GetProperty = function() {
    console.log("GetProperty", arguments);
    throw "URLResponseInfo_GetProperty not implemented";
  };

  var URLResponseInfo_GetBodyAsFileRef = function(res) {
    throw "URLResponseInfo_GetBodyAsFileRef not implemented";
  };


  registerInterface("PPB_URLResponseInfo;1.0", [
    URLResponseInfo_IsURLResponseInfo,
    URLResponseInfo_GetProperty,
    URLResponseInfo_GetBodyAsFileRef
  ]);

})();
