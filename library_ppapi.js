var ppapi_exports = {
  $ppapi_glue: {
    PP_Var: Runtime.generateStructInfo([
      ['i32', 'type'],
      ['i32', 'padding'],
      ['i64', 'value'],
    ]),
    PP_VARTYPE_BOOL: 2,
    PP_VARTYPE_STRING: 5,

    stringForVar: function(p) {
      var o = ppapi_glue.PP_Var;
      var type = {{{ makeGetValue('p + o.type', '0', 'i32') }}};
      if (type != ppapi_glue.PP_VARTYPE_STRING)
        throw "PP_Var is not a string.";
      var uid = {{{ makeGetValue('p + o.value', '0', 'i32') }}};
      return resources.resolve(uid).value;
    },

    boolForVar: function(p) {
      var o = ppapi_glue.PP_Var;
      var type = {{{ makeGetValue('p + o.type', '0', 'i32') }}};
      if (type != ppapi_glue.PP_VARTYPE_BOOL)
        throw "PP_Var is not a Boolean.";
      // PP_Bool is guarenteed to be 4 bytes.
      var value = {{{ makeGetValue('p + o.value', '0', 'i32') }}};
      return value > 0;
    },

    jsForVar: function(p) {
	var o = ppapi_glue.PP_Var;
	var type = {{{ makeGetValue('p + o.type', '0', 'i32') }}};

        if (type == 0) {
	    return undefined;
        } else if (type == 1) {
	    return null;
        } else if (type == ppapi_glue.PP_VARTYPE_BOOL) {
	    // PP_Bool is guarenteed to be 4 bytes.
	    return 0 != {{{ makeGetValue('p + o.value', '0', 'i32') }}};
        } else if (type == 3) {
	    return {{{ makeGetValue('p + o.value', '0', 'i32') }}};
        } else if (type == 4) {
	    return {{{ makeGetValue('p + o.value', '0', 'double') }}};
	} else if (type == ppapi_glue.PP_VARTYPE_STRING) {
	    var uid = {{{ makeGetValue('p + o.value', '0', 'i32') }}};
	    return resources.resolve(uid).value;
	} else {
	    throw "Var type conversion not implemented: " + type;
        }
    }
  },

  Schedule: function(f, p0, p1) {
      setTimeout(function() {
	  _RunScheduled(f, p0, p1);
      }, 10);
  },

  ThrowNotImplemented: function() {
      throw "NotImplemented";
  },

  Console_Log: function(instance, level, value) {
    ppapi.Console.Log(instance, level, ppapi_glue.jsForVar(value));
  },

  Console_LogWithSource: function(instance, level, source, value) {
    ppapi.Console.LogWithSource(instance, level, ppapi_glue.jsForVar(source), ppapi_glue.jsForVar(value));
  },

  Core_AddRefResource: function(uid) {
      resources.addRef(uid);
  },

  Core_ReleaseResource: function(uid) {
      resources.release(uid);
  },

  Messaging_PostMessage: function(instance, value) {
    ppapi.Messaging.PostMessage(instance, ppapi_glue.jsForVar(value));
  },

  URLLoader_Create: function(instance) {
      return resources.register({value: ppapi.URLLoader.Create(instance)});
  },
  URLLoader_IsURLLoader: function() { NotImplemented; },
  URLLoader_Open: function(loader, request, callback) {
      var l = resources.resolve(loader).value;
      var r = resources.resolve(request).value;
      ppapi.URLLoader.Open(l, r, function(status) {
          _RunCompletionCallback(callback, status);
      });
  },
  URLLoader_FollowRedirect: function() { NotImplemented; },
  URLLoader_GetUploadProgress: function() { NotImplemented; },

  URLLoader_GetDownloadProgress: function(loader, bytes_prt, total_ptr) {
      console.log(arguments);
      // HACK not implemented, but don't cause an error.
      return 0;
  },

  URLLoader_GetResponseInfo: function() { NotImplemented; },

  URLLoader_ReadResponseBody: function(loader, buffer_ptr, read_size, callback) {
      var l = resources.resolve(loader).value;
      return ppapi.URLLoader.ReadResponseBody(l, read_size, function(status, data) {
	  writeStringToMemory(data, buffer_ptr, true);
	  _RunCompletionCallback(callback, status);
      });
  },

  URLLoader_FinishStreamingToFile: function() { NotImplemented; },
  URLLoader_Close: function() { NotImplemented; },

  URLRequestInfo_Create: function(instance) {
    return resources.register({value: ppapi.URLRequestInfo.Create(instance)});
  },

  URLRequestInfo_IsURLRequestInfo: function(resource) {
    console.log(resource);
    NotImplemented;
  },

  URLRequestInfo_SetProperty: function(request, property, value) {
    var r = resources.resolve(request).value;
    if (property === 0) {
      r.url = ppapi_glue.stringForVar(value);
    } else if (property === 1) {
      r.method = ppapi_glue.stringForVar(value);
    } else if (property === 5) {
      r.record_download_progress = ppapi_glue.boolForVar(value);
    } else {
      NotImplemented;
    }
  },

  URLRequestInfo_AppendDataToBody: function(request, data, len) {
    console.log(request, data, len);
    NotImplemented;
  },

  URLRequestInfo_AppendFileToBody: function(request, file_ref, start_offset, number_of_bytes, expect_last_time_modified) {
    NotImplemented;
  },

  Var_AddRef: function(v) {
    // TODO check var type.
    var o = ppapi_glue.PP_Var;
    var uid = {{{ makeGetValue('v + o.value', '0', 'i32') }}};
    resources.addRef(uid);
  },

  Var_Release: function(v) {
    // TODO check var type.
    var o = ppapi_glue.PP_Var;
    var uid = {{{ makeGetValue('v + o.value', '0', 'i32') }}};
    resources.release(uid);
  },

  Var_VarFromUtf8: function(result, ptr, len) {
    var value = Pointer_stringify(ptr, len);

    // Create a copy of the string.
    // TODO more efficient copy?
    var memory = _malloc(len + 1);
    for (var i = 0; i < len; i++) {
	HEAPU8[memory + i] = HEAPU8[ptr + i];
    }
    // Null terminate the string because why not?
    HEAPU8[memory + len] = 0;

    var uid = resources.register({
	value: value,
	memory: memory,
	len: len,
	destroy: function() {
	    _free(this.memory)
	}
    });

    // Generate the return value.
    var o = ppapi_glue.PP_Var;
    {{{ makeSetValue('result + o.type', '0', 'ppapi_glue.PP_VARTYPE_STRING', 'i32') }}};
    {{{ makeSetValue('result + o.value', '0', 'uid', 'i32') }}};
  },

  Var_VarToUtf8: function(v, lenptr) {
    var o = ppapi_glue.PP_Var;
    var type = {{{ makeGetValue('v + o.type', '0', 'i32') }}};
    if (type == ppapi_glue.PP_VARTYPE_STRING) {
      var uid = {{{ makeGetValue('v + o.value', '0', 'i32') }}};
      var resource = resources.resolve(uid);
      if (resource) {
        {{{ makeSetValue('lenptr', '0', 'resource.len', 'i32') }}};
        return resource.memory;
      }
    }
    // Something isn't right, return a null pointer.
    {{{ makeSetValue('lenptr', '0', '0', 'i32') }}};
    return 0;
  },
};


autoAddDeps(ppapi_exports, '$ppapi_glue');
mergeInto(LibraryManager.library, ppapi_exports);

