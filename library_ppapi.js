var ppapi_exports = {
  $ppapi_glue: {
    varType: function(p) {
      // p->type is offset 0.
      return getValue(p, 'i32');
    },

    varUID: function(p) {
      // p->value is offset 8.
      return getValue(p + 8, 'i32');
    },

    jsForVar: function(p) {
      var type = ppapi_glue.varType(p);
      var valptr = p + 8;

      if (type == 0) {
        return undefined;
      } else if (type == 1) {
        return null;
      } else if (type == ppapi.PP_VARTYPE_BOOL) {
        // PP_Bool is guarenteed to be 4 bytes.
        return 0 != getValue(valptr, 'i32');
      } else if (type == 3) {
        return getValue(valptr, 'i32');
      } else if (type == 4) {
        return getValue(valptr, 'double');
      } else if (type == ppapi.PP_VARTYPE_STRING) {
        var uid = getValue(valptr, 'i32');
        return resources.resolve(uid, STRING_RESOURCE).value;
      } else {
        throw "Var type conversion not implemented: " + type;
      }
    },

    varForJS: function(p, obj) {
      var typen = (typeof obj);
      var valptr = p + 8;
      if (typen === 'string') {
        var arr = intArrayFromString(obj);
        var memory = allocate(arr, 'i8', ALLOC_NORMAL);
        // Length is adjusted for null terminator.
        var uid = resources.registerString(obj, memory, arr.length-1);
        setValue(p, ppapi.PP_VARTYPE_STRING, 'i32');
        setValue(valptr, uid, 'i32');
      } else if (typen === 'number') {
        // Note this will always pass a double, even when the value can be represented as an int32
        setValue(p, ppapi.PP_VARTYPE_DOUBLE, 'i32');
        setValue(valptr, obj, 'double');
      } else if (typen === 'boolean') {
        setValue(p, ppapi.PP_VARTYPE_BOOL, 'i32');
        setValue(valptr, obj ? 1 : 0, 'i32');
      } else if (typen === 'undefined') {
        setValue(p, ppapi.PP_VARTYPE_UNDEFINED, 'i32');
      } else if (obj === null) {
        setValue(p, ppapi.PP_VARTYPE_NULL, 'i32');
      } else if (obj instanceof ArrayBuffer) {
        var memory = _malloc(obj.byteLength);
        // Note: "buffer" is an implementation detail of Emscripten and is likely not a stable interface.
        var memory_view = new Int8Array(buffer, memory, obj.byteLength);
        memory_view.set(new Int8Array(obj));
        var uid = resources.registerArrayBuffer(memory, obj.byteLength);
        setValue(p, ppapi.PP_VARTYPE_ARRAY_BUFFER, 'i32');
        setValue(valptr, uid, 'i32');
      } else {
        throw "Var type conversion not implemented: " + typen;
      }
    },

    varForJSInt: function(p, obj) {
      var typen = (typeof obj);
      var valptr = p + 8;
      if (typen === 'number') {
        setValue(p, ppapi.PP_VARTYPE_INT32, 'i32');
        setValue(valptr, obj, 'i32');
      } else {
        throw "Not an integer: " + typen;
      }
    },

    convertCompletionCallback: function(callback) {
      // Assumes 4-byte pointers.
      var func = getValue(callback, 'i32');
      var user_data = getValue(callback + 4, 'i32');
      // TODO correct way to call?
      return function(result) {
        if (typeof result !== 'number') {
          throw "Invalid argument to callback: " + result;
        }

        Runtime.dynCall('vii', func, [user_data, result]);
      };
    },
  },

  GetBrowserInterface: function(interface_name) {
    var name = Pointer_stringify(interface_name);
    if (!(name in interfaces)) {
      console.error('Requested unknown interface: ' + name);
      return 0;
    }
    var inf = interfaces[name]|0;
    if (inf === 0) {
      // The interface exists, but it is not available for this particular browser.
      console.error('Requested unavailable interface: ' + name);
    }
    return inf;
  },
};


autoAddDeps(ppapi_exports, '$ppapi_glue');
mergeInto(LibraryManager.library, ppapi_exports);
