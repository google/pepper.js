var ppapi_exports = {
  $ppapi_glue: {
    PP_Var: Runtime.generateStructInfo([
      ['i32', 'type'],
      ['i32', 'padding'],
      ['i64', 'value'],
    ]),

    jsForVar: function(p) {
      var o = ppapi_glue.PP_Var;
      var type = {{{ makeGetValue('p + o.type', '0', 'i32') }}};

      if (type == 0) {
	return undefined;
      } else if (type == 1) {
	return null;
      } else if (type == ppapi.PP_VARTYPE_BOOL) {
	// PP_Bool is guarenteed to be 4 bytes.
	return 0 != {{{ makeGetValue('p + o.value', '0', 'i32') }}};
      } else if (type == 3) {
	return {{{ makeGetValue('p + o.value', '0', 'i32') }}};
      } else if (type == 4) {
	return {{{ makeGetValue('p + o.value', '0', 'double') }}};
      } else if (type == ppapi.PP_VARTYPE_STRING) {
	var uid = {{{ makeGetValue('p + o.value', '0', 'i32') }}};
	return resources.resolve(uid, STRING_RESOURCE).value;
      } else {
	throw "Var type conversion not implemented: " + type;
      }
    },

    varType: function(p) {
      var o = ppapi_glue.PP_Var;
      return {{{ makeGetValue('p + o.type', '0', 'i32') }}};
    },

    varForJS: function(p, obj) {
      var o = ppapi_glue.PP_Var;
      var typen = (typeof obj);
      if (typen === 'string') {
        var arr = intArrayFromString(obj);
        var memory = allocate(arr, 'i8', ALLOC_NORMAL);
        // Length is adjusted for null terminator.
        var uid = resources.registerString(obj, memory, arr.length-1);
        {{{ makeSetValue('p + o.type', '0', '5', 'i32') }}};
        {{{ makeSetValue('p + o.value', '0', 'uid', 'i32') }}};
      } else if (typen === 'number') {
        // Note this will always pass a double, even when the value can be represented as an int32
        {{{ makeSetValue('p + o.type', '0', '4', 'i32') }}};
        {{{ makeSetValue('p + o.value', '0', 'obj', 'double') }}};
      } else if (typen === 'boolean') {
        var value = (obj) ? 1 : 0;
        {{{ makeSetValue('p + o.type', '0', '2', 'i32') }}};
        {{{ makeSetValue('p + o.value', '0', 'value', 'i32') }}};
      } else if (typen === 'undefined') {
        {{{ makeSetValue('p + o.type', '0', '0', 'i32') }}};
      } else if (obj === null) {
        {{{ makeSetValue('p + o.type', '0', '1', 'i32') }}};
      } else if (obj instanceof ArrayBuffer) {
        var memory = _malloc(obj.byteLength);
        // Note: "buffer" is an implementation detail of Emscripten and is likely not a stable interface.
        var memory_view = new Int8Array(buffer, memory, obj.byteLength);
        memory_view.set(new Int8Array(obj));
        var uid = resources.registerArrayBuffer(memory, obj.byteLength);
        {{{ makeSetValue('p + o.type', '0', '9', 'i32') }}};
        {{{ makeSetValue('p + o.value', '0', 'uid', 'i32') }}};
      } else {
        throw "Var type conversion not implemented: " + typen;
      }
    },

    varForJSInt: function(p, obj) {
      var o = ppapi_glue.PP_Var;
      var typen = (typeof obj);
      if (typen === 'number') {
        {{{ makeSetValue('p + o.type', '0', '3', 'i32') }}};
        {{{ makeSetValue('p + o.value', '0', 'obj', 'i32') }}};
      } else {
        throw "Not an integer: " + typen;
      }
    },

    convertCompletionCallback: function(callback) {
      // Assumes 4-byte pointers.
      var func = {{{ makeGetValue('callback + 0', '0', 'i32') }}};
      var user_data = {{{ makeGetValue('callback + 4', '0', 'i32') }}};
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
