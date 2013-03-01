var ppapi_exports = {
  $ppapi_glue: {
    PP_Var: Runtime.generateStructInfo([
      ['i32', 'type'],
      ['i32', 'padding'],
      ['i64', 'value'],
    ]),
    PP_VARTYPE_BOOL: 2,
    PP_VARTYPE_STRING: 5,
    var_tracker: {},
    var_uid: 0,

    stringForVar: function(p) {
      var o = ppapi_glue.PP_Var;
      var type = {{{ makeGetValue('p + o.type', '0', 'i32') }}};
      if (type != ppapi_glue.PP_VARTYPE_STRING)
        throw "PP_Var is not a string.";
      var uid = {{{ makeGetValue('p + o.value', '0', 'i32') }}};
      var resource = ppapi_glue.var_tracker[uid];
      if (!resource) {
        throw "Tried to reference a dead PP_Var.";
      }
      return resource.value;
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

    allocateUID: function() {
      while (ppapi_glue.var_uid in ppapi_glue.var_tracker) {
        ppapi_glue.var_uid = ppapi_glue.var_uid + 1 & 0xffffffff;
      }
      return ppapi_glue.var_uid;
    },

    createResource: function(value) {
      var uid = ppapi_glue.allocateUID();
      ppapi_glue.var_tracker[uid] = {refcount: 1, value: value};
      return uid;
    }
  },

  ThrowNotImplemented: function() {
      throw "NotImplemented";
  },

  Console_Log: function(instance, level, value) {
    ppapi.Console.Log(instance, level, ppapi_glue.stringForVar(value));
  },

  Console_LogWithSource: function(instance, level, source, value) {
    ppapi.Console.LogWithSource(instance, level, ppapi_glue.stringForVar(source), ppapi_glue.stringForVar(value));
  },

  Core_ReleaseResource: function(uid) {
      console.log("Releasing resource", uid);
      var resource = ppapi_glue.var_tracker[uid];
      if (!resource) {
        throw "Tried to reference a dead resource.";
      }
      console.log(resource);
      resource.refcount -= 1;
      if (resource.refcount <= 0) {
        delete ppapi_glue.var_tracker[uid];
      }
  },

  Messaging_PostMessage: function(instance, value) {
    // TODO arbitrary objects.
    ppapi.Messaging.PostMessage(instance, ppapi_glue.stringForVar(value));
  },

  URLLoader_Create: function(instance) {
      return ppapi_glue.createResource(ppapi.URLLoader.Create(instance));
  },
  URLLoader_IsURLLoader: function() { NotImplemented; },
  URLLoader_Open: function(loader, request, callback) {
      var l = ppapi_glue.var_tracker[loader].value;
      var r = ppapi_glue.var_tracker[request].value;
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
      var l = ppapi_glue.var_tracker[loader].value;
      return ppapi.URLLoader.ReadResponseBody(l, read_size, function(status, data) {
	  writeStringToMemory(data, buffer_ptr, true);
	  _RunCompletionCallback(callback, status);
      });
  },

  URLLoader_FinishStreamingToFile: function() { NotImplemented; },
  URLLoader_Close: function() { NotImplemented; },

  URLRequestInfo_Create: function(instance) {
    return ppapi_glue.createResource(ppapi.URLRequestInfo.Create(instance));
  },

  URLRequestInfo_IsURLRequestInfo: function(resource) {
    console.log(resource);
    NotImplemented;
  },

  URLRequestInfo_SetProperty: function(request, property, value) {
    var r = ppapi_glue.var_tracker[request].value;
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
    var resource = ppapi_glue.var_tracker[uid];
    if (resource) {
      resource.refcount += 1;
    }
  },

  Var_Release: function(v) {
    // TODO check var type.
    var o = ppapi_glue.PP_Var;
    var uid = {{{ makeGetValue('v + o.value', '0', 'i32') }}};
    var resource = ppapi_glue.var_tracker[uid];
    if (resource) {
      resource.refcount -= 1;
      if (resource.refcount <= 0) {
        _free(ppapi_glue.var_tracker[uid].memory);
        delete ppapi_glue.var_tracker[uid];
      }
    }
  },

  Var_VarFromUtf8: function(result, ptr, len) {
    var value = Pointer_stringify(ptr, len);
    var uid = ppapi_glue.allocateUID();

    // Create a copy of the string.
    // TODO more efficient copy?
    var memory = _malloc(len + 1);
    for (var i = 0; i < len; i++) {
	HEAPU8[memory + i] = HEAPU8[ptr + i];
    }
    // Null terminate the string because why not?
    HEAPU8[memory + len] = 0;

    ppapi_glue.var_tracker[uid] = {refcount: 1, value: value, memory: memory, len: len};

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
      var resource = ppapi_glue.var_tracker[uid];
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

