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
    },
    convertCompletionCallback: function(callback) {
      // Assumes 4-byte pointers.
      var func = {{{ makeGetValue('callback + 0', '0', 'i32') }}};
      var user_data = {{{ makeGetValue('callback + 4', '0', 'i32') }}};
      // TODO correct way to call?
      return function(result) { FUNCTION_TABLE[func](user_data, result); };
    },
    setRect: function(rect, ptr) {
	{{{ makeSetValue('ptr', '0', 'rect.x', 'i32') }}};
	{{{ makeSetValue('ptr + 4', '0', 'rect.y', 'i32') }}};
	{{{ makeSetValue('ptr + 8', '0', 'rect.width', 'i32') }}};
	{{{ makeSetValue('ptr + 12', '0', 'rect.height', 'i32') }}};
    },
    getSize: function(ptr) {
	return {
	    width: {{{ makeGetValue('ptr', '0', 'i32') }}},
	    height: {{{ makeGetValue('ptr + 4', '0', 'i32') }}}
	};
    },
    getPos: function(ptr) {
	return {
	    x: {{{ makeGetValue('ptr', '0', 'i32') }}},
	    y: {{{ makeGetValue('ptr + 4', '0', 'i32') }}}
	};
    },
  },

  Schedule: function(f, p0, p1) {
      setTimeout(function() {
	  _RunScheduled(f, p0, p1);
      }, 0);
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

  Core_CallOnMainThread: function(delay, callback, result) {
      var js_callback = ppapi_glue.convertCompletionCallback(callback);
      setTimeout(function() {
          js_callback(result);
      }, delay);
  },

  Core_AddRefResource: function(uid) {
      resources.addRef(uid);
  },

  Core_ReleaseResource: function(uid) {
      resources.release(uid);
  },

    Core_GetTime: function() {
	return (new Date()) / 1000;
    },

    Graphics2D_Create: function(instance, size_ptr, is_always_opaque) {
	var resource = resources.register("graphics_2d", {size: ppapi_glue.getSize(size_ptr), always_opaque: true});
	return resource;
    },
    Graphics2D_IsGraphics2D: function(resource) {
	return resources.is(resource, "graphics_2d");
    },
    Graphics2D_Describe: function(resource, size_ptr, is_always_opqaue_ptr) {
	NotImplemented;
    },
    Graphics2D_PaintImageData: function(resource, image_data, top_left_ptr, src_rect_ptr) {
	var res = resources.resolve(image_data);
	res.image_data.data.set(res.view);

	var top_left = ppapi_glue.getPos(top_left_ptr);
        ctx.putImageData(res.image_data, top_left.x, top_left.y);
    },
    Graphics2D_Scroll: function(resource, clip_rect_ptr, amount_ptr) {
	NotImplemented;
    },
    Graphics2D_ReplaceContents: function(resource, image_data) {
	NotImplemented;
    },
    Graphics2D_Flush: function(resource, callback) {
	// Ignore
	// TODO vsync
	var c = ppapi_glue.convertCompletionCallback(callback);
	setTimeout(function() {
	    c(ppapi.PP_Error.PP_OK);
	}, 15);
	return ppapi.PP_Error.PP_OK;
   },


    ImageData_GetNativeImageDataFormat: function() {
	NotImplemented;
    },
    ImageData_IsImageDataFormatSupported: function(format) {
	NotImplemented;
    },
    ImageData_Create: function(instance, format, size_ptr, init_to_zero) {
	var size = ppapi_glue.getSize(size_ptr);
	var bytes = size.width * size.height * 4
        var memory = _malloc(bytes);
        // Note: "buffer" is an implementation detail of Emscripten and is likely not a stable interface.
        var view = new Uint8ClampedArray(buffer, memory, bytes);
        // Due to limitations of the canvas API, we need to create an intermediate "ImageData" buffer.
	var image_data = ctx.createImageData(size.width, size.height);
	var uid = resources.register("image_data", {
            format: format,
            size: size,
            memory: memory,
            view: view,
            image_data: image_data,
            destroy: function() {
                throw "Destroying image data not implemented!";
            }
	});
	return uid;
    },
    ImageData_IsImageData: function (image_data) {
	return resources.is("image_data", image_data);
    },
    ImageData_Describe: function(image_data, desc_ptr) {
	//if (!resources.is("image_data", image_data)) return 0;

	var res = resources.resolve(image_data);
	{{{ makeSetValue('desc_ptr + 0', '0', 'res.format', 'i32') }}};
	{{{ makeSetValue('desc_ptr + 4', '0', 'res.size.width', 'i32') }}};
	{{{ makeSetValue('desc_ptr + 8', '0', 'res.size.height', 'i32') }}};
	{{{ makeSetValue('desc_ptr + 12', '0', '(res.size.width*4)', 'i32') }}};
	return 1;
    },
    ImageData_Map: function(image_data) {
	return resources.resolve(image_data).memory;
    },
    ImageData_Unmap: function(image_data) {
	// Ignore
    },

    Instance_BindGraphics: function(instance, device) {
	// Ignore
	return 1;
    },

    Instance_IsFullFrame: function(instance) {
	return 0;
    },


  Messaging_PostMessage: function(instance, value) {
    ppapi.Messaging.PostMessage(instance, ppapi_glue.jsForVar(value));
  },

  URLLoader_Create: function(instance) {
      return resources.register("url_loader", ppapi.URLLoader.Create(instance));
  },
  URLLoader_IsURLLoader: function(resource) {
      return resources.is(resource, "url_loader");
  },
  URLLoader_Open: function(loader, request, callback) {
      var l = resources.resolve(loader);
      var r = resources.resolve(request);
      var c = ppapi_glue.convertCompletionCallback(callback);
      ppapi.URLLoader.Open(l, r, c);
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
      var l = resources.resolve(loader);
      var c = ppapi_glue.convertCompletionCallback(callback);
      return ppapi.URLLoader.ReadResponseBody(l, read_size, function(status, data) {
	  writeStringToMemory(data, buffer_ptr, true);
	  c(status);
      });
  },

  URLLoader_FinishStreamingToFile: function() { NotImplemented; },
  URLLoader_Close: function() { NotImplemented; },

  URLRequestInfo_Create: function(instance) {
      return resources.register("url_request_info", ppapi.URLRequestInfo.Create(instance));
  },

  URLRequestInfo_IsURLRequestInfo: function(resource) {
      return resources.is(resource, "url_request_info");
  },

  URLRequestInfo_SetProperty: function(request, property, value) {
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

    var uid = resources.register("string", {
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

    View_IsView: function(resource) {
	return resources.is(resource, "view");
    },
    View_GetRect: function(resource, rectptr) {
	ppapi_glue.setRect(resources.resolve(resource).rect, rectptr);
	return true;
    },
    View_IsFullscreen: function(resource) {
	return resources.resolve(resource).fullscreen;
    },
    View_IsVisible: function(resource) {
	return resources.resolve(resource).visible;
    },
    View_IsPageVisible: function(resource) {
	return resources.resolve(resource).page_visible;
    },
    View_GetClipRect: function(resource, rectptr) {
	ppapi_glue.setRect(resources.resolve(resource).clip_rect, rectptr);
	return true;
    },
};


autoAddDeps(ppapi_exports, '$ppapi_glue');
mergeInto(LibraryManager.library, ppapi_exports);
