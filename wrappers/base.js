(function() {
  var DoLog = function(level, value) {
    // TODO enum?
    if (level == 2) {
      console.warn(value);
    } else if (level == 3) {
      console.error(value);
    } else {
      console.log(value);
    }
  }

  var Console_Log = function(instance, level, value) {
    DoLog(level, ppapi_glue.jsForVar(value));
  };

  var Console_LogWithSource = function(instance, level, source, value) {
    DoLog(level, ppapi_glue.jsForVar(source) + ": " + ppapi_glue.jsForVar(value));
  };

  registerInterface("PPB_Console;1.0", [
    Console_Log,
    Console_LogWithSource
  ]);


  var Core_AddRefResource = function(uid) {
    resources.addRef(uid);
  };

  var Core_ReleaseResource = function(uid) {
    resources.release(uid);
  };

  var Core_GetTime = function() {
    return (new Date()) / 1000;
  };

  var Core_GetTimeTicks = function() {
    return performance.now() / 1000;
  };

  var Core_CallOnMainThread = function(delay, callback, result) {
    var js_callback = glue.getCompletionCallback(callback);
    setTimeout(function() {
      js_callback(result);
    }, delay);
  };

  var Core_IsMainThread = function() {
    return 1;
  };

  registerInterface("PPB_Core;1.0", [
    Core_AddRefResource,
    Core_ReleaseResource,
    Core_GetTime,
    Core_GetTimeTicks,
    Core_CallOnMainThread,
    Core_IsMainThread
  ]);


  var Instance_BindGraphics = function(instance, device) {
    var inst = resources.resolve(instance, INSTANCE_RESOURCE);
    if (inst === undefined) {
      return 0;
    }
    if (device === 0) {
        inst.unbind();
        return 1;
    }
    var dev = resources.resolve(device, GRAPHICS_3D_RESOURCE, true) || resources.resolve(device, GRAPHICS_2D_RESOURCE, true);
    if (dev === undefined) {
        return 0;
    }
    inst.bind(dev);
    return 1;
  };

  var Instance_IsFullFrame = function(instance) {
    // Only true for MIME handlers - which are not supported.
    return 0;
  };

  registerInterface("PPB_Instance;1.0", [
    Instance_BindGraphics,
    Instance_IsFullFrame
  ]);


  var Messaging_PostMessage = function(instance, value) {
    var inst = resources.resolve(instance, INSTANCE_RESOURCE);
    if (inst == undefined) {
      return;
    }
    var val = ppapi_glue.jsForVar(value);
    var evt = document.createEvent('Event');
    evt.initEvent('message', true, true);  // bubbles, cancelable
    evt.data = val;
    inst.element.dispatchEvent(evt);
  };

  registerInterface("PPB_Messaging;1.0", [
    Messaging_PostMessage
  ]);

  var isRefCountedVarType = function(type) {
    return type >= 5;
  };

  var Var_AddRef = function(v) {
    if (isRefCountedVarType(ppapi_glue.varType(v))) {
      resources.addRef(ppapi_glue.varUID(v));
    }
  };

  var Var_Release = function(v) {
    if (isRefCountedVarType(ppapi_glue.varType(v))) {
      resources.release(ppapi_glue.varUID(v));
    }
  };

  var Var_VarFromUtf8_1_0 = function(result, module, ptr, len) {
    Var_VarFromUtf8_1_1(result, ptr, len);
  };

  var Var_VarFromUtf8_1_1 = function(result, ptr, len) {
    var value = glue.decodeUTF8(ptr, len);

    // Not a valid UTF-8 string.  Return null.
    if (value === null) {
      ppapi_glue.varForJS(result, null);
      return
    }

    // Create a copy of the string.
    // TODO more efficient copy?
    var memory = _malloc(len + 1);
    for (var i = 0; i < len; i++) {
      HEAPU8[memory + i] = HEAPU8[ptr + i];
    }
    // Null terminate the string because why not?
    HEAPU8[memory + len] = 0;

    // Generate the return value.
    setValue(result, ppapi.PP_VARTYPE_STRING, 'i32');
    setValue(result + 8, resources.registerString(value, memory, len), 'i32');
  };

  var Var_VarToUtf8 = function(v, lenptr) {
    // Defensively set the length to zero so that we can early out at any point.
    setValue(lenptr, 0, 'i32');

    if (ppapi_glue.varType(v) !== ppapi.PP_VARTYPE_STRING) {
      return 0;
    }
    var uid = ppapi_glue.varUID(v);
    var resource = resources.resolve(uid, STRING_RESOURCE);
    if (resource === undefined) {
      return 0;
    }
    setValue(lenptr, resource.len, 'i32');
    return resource.memory;
  };

  registerInterface("PPB_Var;1.0", [
    Var_AddRef,
    Var_Release,
    Var_VarFromUtf8_1_0,
    Var_VarToUtf8
  ]);

  // TODO eliminate redundantly defined function pointers?
  registerInterface("PPB_Var;1.1", [
    Var_AddRef,
    Var_Release,
    Var_VarFromUtf8_1_1,
    Var_VarToUtf8
  ]);

  var VarArrayBuffer_Create = function(var_ptr, size_in_bytes) {
    throw "VarArrayBuffer_Create not implemented";
  }

  var VarArrayBuffer_ByteLength = function(var_ptr, byte_length_ptr) {
    throw "VarArrayBuffer_ByteLength not implemented";
  }

  var VarArrayBuffer_Map = function(var_ptr) {
    if (ppapi_glue.varType(var_ptr) !== ppapi.PP_VARTYPE_ARRAY_BUFFER) {
      return 0;
    }
    var uid = ppapi_glue.varUID(var_ptr);
    var resource = resources.resolve(uid, ARRAY_BUFFER_RESOURCE);
    if (resource === undefined) {
      return 0;
    }
    return resource.memory;
  }

  var VarArrayBuffer_Unmap = function(var_ptr) {
    // Currently a nop because the data is always mapped.
  }

  registerInterface("PPB_VarArrayBuffer;1.0", [
    VarArrayBuffer_Create,
    VarArrayBuffer_ByteLength,
    VarArrayBuffer_Map,
    VarArrayBuffer_Unmap
  ]);

})();
