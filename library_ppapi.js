var ppapi_exports = {
  $ppapi_glue: {
    PP_Var: Runtime.generateStructInfo([
      ['i32', 'type'],
      ['i32', 'padding'],
      ['i64', 'value'],
    ]),
    PP_VARTYPE_STRING: 5,
    var_tracker: {},
    var_uid: 0,
    stringForVar: function(p) {
      var o = ppapi_glue.PP_Var;
      var type = Module.getValue(p + o.type, 'i32');
      if (type != ppapi_glue.PP_VARTYPE_STRING)
        throw "PP_Var is not a string.";
      var uid = Module.getValue(p + o.value, 'i32');
      var resource = ppapi_glue.var_tracker[uid];
      if (!resource) {
        throw "Tried to reference a dead PP_Var.";
      }
      return resource.value;
    },
  },

  NotImplemented: function() {
    throw "Function not implemented.";
  },

  Console_Log: function(instance, level, value) {
    var svalue = ppapi_glue.stringForVar(value);
    // TODO symbols?
    if (level == 2) {
      console.warn(svalue);
    } else if (level == 3) {
      console.error(svalue);
    } else {
      console.log(svalue);
    }

  },

  Console_LogWithSource: function() {
    throw "LogWithSource not implemented.";
  },

  Messaging_PostMessage: function(instance, value) {
    var svalue = ppapi_glue.stringForVar(value);
    Module.print("PostMessage: " + svalue);
  },

  Var_AddRef: function(v) {
    // TODO check var type.
    var o = ppapi_glue.PP_Var;
    var uid = Module.getValue(v + o.value, 'i32');
    var resource = ppapi_glue.var_tracker[uid];
    if (resource) {
      resource.refcount += 1;
    }
  },

  Var_Release: function(v) {
    // TODO check var type.
    var o = ppapi_glue.PP_Var;
    var uid = Module.getValue(v + o.value, 'i32');
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
    while (ppapi_glue.var_uid in ppapi_glue.var_tracker) {
      ppapi_glue.var_uid = ppapi_glue.var_uid + 1 & 0xffffffff;
    }
    var uid = ppapi_glue.var_uid;

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
    Module.setValue(result + o.type, ppapi_glue.PP_VARTYPE_STRING, 'i32');
    Module.setValue(result + o.value, uid, 'i32');
  },

  Var_VarToUtf8: function(v, lenptr) {
    var o = ppapi_glue.PP_Var;
    var type = Module.getValue(v + o.type, 'i32');
    if (type == ppapi_glue.PP_VARTYPE_STRING) {
      var uid = Module.getValue(v + o.value, 'i32');
      var resource = ppapi_glue.var_tracker[uid];
      if (resource) {
        Module.setValue(lenptr, resource.len, 'i32');
        return resource.memory;
      }
    }
    // Something isn't right, return a null pointer.
    Module.setValue(lenptr, 0, 'i32');
    return 0;
  },
};


autoAddDeps(ppapi_exports, '$ppapi_glue');
mergeInto(LibraryManager.library, ppapi_exports);

