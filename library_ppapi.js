// C functions called by the JavaScript shims.
EXPORTED_FUNCTIONS['_DoPostMessage'] = 1;
EXPORTED_FUNCTIONS['_DoChangeView'] = 1;
EXPORTED_FUNCTIONS['_NativeCreateInstance'] = 1;

// Actually a JS function. Done to fix minimization problems.
EXPORTED_FUNCTIONS['CreateInstance'] = 1;

// HACK PPAPI will export a bunch of anonymous function pointers.
// Eventually we should make these non-anonymous.
RESERVED_FUNCTION_POINTERS = 1000;

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
      return function(result) {
        Runtime.dynCall('vii', func, [user_data, result]);
      };
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

  GetBrowserInterface: function(interface_name) {
      var name = Pointer_stringify(interface_name);
      var inf = interfaces[name]|0;
      if (inf == 0) {
	  console.error('Requested unknown interface: ' + name);
      }
      return inf;
  },
};


autoAddDeps(ppapi_exports, '$ppapi_glue');
mergeInto(LibraryManager.library, ppapi_exports);
