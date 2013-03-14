(function() {
    var URLLoader_Create = function(instance) {
	return resources.register("url_loader", ppapi.URLLoader.Create(instance));
    };

    var URLLoader_IsURLLoader = function(resource) {
	return resources.is(resource, "url_loader");
    };

    var URLLoader_Open = function(loader, request, callback) {
	var l = resources.resolve(loader);
	var r = resources.resolve(request);
	var c = ppapi_glue.convertCompletionCallback(callback);
	ppapi.URLLoader.Open(l, r, c);
    };

    var URLLoader_FollowRedirect = function() { NotImplemented; };
    var URLLoader_GetUploadProgress = function() { NotImplemented; };

    var URLLoader_GetDownloadProgress = function(loader, bytes_prt, total_ptr) {
	console.log(arguments);
	// HACK not implemented, but don't cause an error.
	return 0;
    };

    var URLLoader_GetResponseInfo = function() { NotImplemented; };

    var URLLoader_ReadResponseBody = function(loader, buffer_ptr, read_size, callback) {
	var l = resources.resolve(loader);
	var c = ppapi_glue.convertCompletionCallback(callback);
	return ppapi.URLLoader.ReadResponseBody(l, read_size, function(status, data) {
	    writeStringToMemory(data, buffer_ptr, true);
	    c(status);
	});
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
	return resources.register("url_request_info", ppapi.URLRequestInfo.Create(instance));
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