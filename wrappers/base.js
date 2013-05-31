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
    var js_callback = ppapi_glue.convertCompletionCallback(callback);
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
    return 1;
  };

  var Instance_IsFullFrame = function(instance) {
    // TODO
    return 0;
  };

  registerInterface("PPB_Instance;1.0", [
    Instance_BindGraphics,
    Instance_IsFullFrame
  ]);


  var Messaging_PostMessage = function(instance, value) {
    var inst = resources.resolve(instance);
    var val = ppapi_glue.jsForVar(value);
    var evt = document.createEvent('Event');
    evt.initEvent('message', true, true);  // bubbles, cancelable
    evt.data = val;
    inst.element.dispatchEvent(evt);
  };

  registerInterface("PPB_Messaging;1.0", [
    Messaging_PostMessage
  ]);


  var decodeUTF8 = function(ptr, len) {
    var chars = [];
    var i = 0;
    var val;
    var n;
    var b;
    while (i < len) {
      b = HEAPU8[ptr + i];
      i += 1;
      if (b < 0x80) {
	val = b;
	n = 0;
      } else if ((b & 0xE0) === 0xC0) {
	val = b & 0x1f;
	n = 1;
      } else if ((b & 0xF0) === 0xE0) {
	val = b & 0x0f;
	n = 2;
      } else if ((b & 0xF8) === 0xF0) {
	val = b & 0x07;
	n = 3;
      } else if ((b & 0xFC) === 0xF8) {
	val = b & 0x03;
	n = 4;
      } else if ((b & 0xFE) === 0xFC) {
	val = b & 0x01;
	n = 5;
      } else {
	return null;
      }
      if (i + n > len) {
	return null;
      }
      while (n > 0) {
	b = HEAPU8[ptr + i];
	if ((b & 0xC0) !== 0x80) {
	  return null;
	}
	val = (val << 6) | (b & 0x3f);
	i += 1;
	n -= 1;
      }
      chars.push(String.fromCharCode(val));
    }
    return chars.join("");
  };

  var Var_AddRef = function(v) {
    // TODO check var type.
    var o = ppapi_glue.PP_Var;
    var uid = getValue(v + o.value, 'i32');
    resources.addRef(uid);
  };

  var Var_Release = function(v) {
    // TODO check var type.
    var o = ppapi_glue.PP_Var;
    var uid = getValue(v + o.value, 'i32');
    resources.release(uid);
  };

  var Var_VarFromUtf8_1_0 = function(result, module, ptr, len) {
    Var_VarFromUtf8_1_1(result, ptr, len);
  };

  var Var_VarFromUtf8_1_1 = function(result, ptr, len) {
    var o = ppapi_glue.PP_Var;
    var value = decodeUTF8(ptr, len);

    // Not a valid UTF-8 string.  Return null.
    if (value === null) {
      setValue(result + o.type, ppapi_glue.PP_VARTYPE_NULL, 'i32');
      setValue(result + o.value, 0, 'i32');
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

    var uid = resources.register("string", {
      value: value,
      memory: memory,
      len: len,
      destroy: function() {
	_free(this.memory)
      }
    });

    // Generate the return value.
    setValue(result + o.type, ppapi_glue.PP_VARTYPE_STRING, 'i32');
    setValue(result + o.value, uid, 'i32');
  };

  var Var_VarToUtf8 = function(v, lenptr) {
    var o = ppapi_glue.PP_Var;
    var type = getValue(v + o.type, 'i32');
    if (type == ppapi_glue.PP_VARTYPE_STRING) {
      var uid = getValue(v + o.value, 'i32');
      var resource = resources.resolve(uid);
      if (resource) {
	setValue(lenptr, resource.len, 'i32');
	return resource.memory;
      }
    }
    // Something isn't right, return a null pointer.
    setValue(lenptr, 0, 'i32');
    return 0;
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
})();
