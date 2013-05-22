(function() {
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
	var uid = resources.register("url_loader", {});
	return uid;
    };

    var URLLoader_IsURLLoader = function(resource) {
	return resources.is(resource, "url_loader");
    };

    var URLLoader_Open = function(loader, request, callback) {
	loader = resources.resolve(loader);
	request = resources.resolve(request);
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
		callback(ppapi.PP_Error.PP_OK);
		did_callback = true;
	    }
        }
	req.onreadystatechange = function() {
	    if (this.readyState == 1) {
	    } else if (this.readyState == 2) {
		if (this.status != 200) {
		    callback(ppapi.PP_Error.PP_FAILED);
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

	return ppapi.PP_Error.PP_OK_COMPLETIONPENDING;
    };

    var URLLoader_FollowRedirect = function() { NotImplemented; };
    var URLLoader_GetUploadProgress = function() { NotImplemented; };

    var URLLoader_GetDownloadProgress = function(loader, bytes_ptr, total_ptr) {
	var l = resources.resolve(loader);
	setValue(bytes_ptr, l.progress_bytes, 'i64');
	setValue(total_ptr, l.progress_total, 'i64');
	return 1;
    };

    var URLLoader_GetResponseInfo = function() { NotImplemented; };

    var URLLoader_ReadResponseBody = function(loader, buffer_ptr, read_size, callback) {
	var loader = resources.resolve(loader);
	var c = ppapi_glue.convertCompletionCallback(callback);

	loader.pendingReadCallback = function(status, data) {
            HEAP8.set(data, buffer_ptr);
	    c(status);
	};
	loader.pendingReadSize = read_size;
	setTimeout(function() { updatePendingRead(loader); }, 0);
	return ppapi.PP_Error.PP_OK_COMPLETIONPENDING;

    };

    var URLLoader_FinishStreamingToFile = function() { NotImplemented; };
    var URLLoader_Close = function() { NotImplemented; };

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
	return resources.register("url_request_info", {});
    };

    var URLRequestInfo_IsURLRequestInfo = function(resource) {
	return resources.is(resource, "url_request_info");
    };

    var URLRequestInfo_SetProperty = function(request, property, value) {
	var r = resources.resolve(request);
	if (property === 0) {
	    r.url = ppapi_glue.stringForVar(value);
	} else if (property === 1) {
	    r.method = ppapi_glue.stringForVar(value);
	} else if (property === 5) {
	    r.record_download_progress = ppapi_glue.boolForVar(value);
	} else {
	    NotImplemented;
	}
    };

    var URLRequestInfo_AppendDataToBody = function(request, data, len) {
	console.log(request, data, len);
	NotImplemented;
    };

    var URLRequestInfo_AppendFileToBody = function(request, file_ref, start_offset, number_of_bytes, expect_last_time_modified) {
	NotImplemented;
    };

    registerInterface("PPB_URLRequestInfo;1.0", [
	URLRequestInfo_Create,
	URLRequestInfo_IsURLRequestInfo,
	URLRequestInfo_SetProperty,
	URLRequestInfo_AppendDataToBody,
	URLRequestInfo_AppendFileToBody
    ]);
})();