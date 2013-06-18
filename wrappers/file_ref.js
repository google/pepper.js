(function() {

  var FILE_REF_RESOURCE = "file_ref";

  var DummyError = function(error) {
    console.log('Unhandled fileref error!', error);
    throw 'Unhandled fileref error!' + error;
  };

  var FileRef_Create = function(file_system, path_ptr) {
    var path = util.decodeUTF8(path_ptr);
    if (path === null) {
      // Not UTF8
      return 0;
    }
    resources.addRef(file_system);
    return resources.register(FILE_REF_RESOURCE, {
        path: path,
        file_system: file_system,
        destroy: function () {
          resources.release(file_system);
        }
      });
  };

  var FileRef_IsFileRef = function(res) {
    return resources.is(res, FILE_REF_RESOURCE);
  };

  var FileRef_GetFileSystemType = function() {
    throw "FileRef_GetFileSystemType not implemented";
  };

  var FileRef_GetName = function() {
    throw "FileRef_GetName not implemented";
  };

  var FileRef_GetPath = function() {
    throw "FileRef_GetPath not implemented";
  };

  var FileRef_GetParent = function() {
    throw "FileRef_GetParent not implemented";
  };

  var FileRef_MakeDirectory = function() {
    throw "FileRef_MakeDirectory not implemented";
  };

  var FileRef_Touch = function() {
    throw "FileRef_Touch not implemented";
  };

  var FileRef_Delete = function(file_ref, callback_ptr) {
    var callback = ppapi_glue.convertCompletionCallback(callback_ptr);
    var ref = resources.resolve(file_ref, FILE_REF_RESOURCE);
    if (ref === undefined) {
      return ppapi.PP_Error.PP_ERROR_BADRESOURCE;
    }
    var file_system = resources.resolve(ref.file_system);
    if (file_system  === undefined) {
      // This is an internal error.
      return ppapi.PP_Error.PP_ERROR_FAILED;
    }

    var error_handler = function(error) {
      var code = error.code;
      if (code === FileError.NOT_FOUND_ERR) {
        callback(ppapi.PP_Error.PP_ERROR_FILENOTFOUND);
      } else {
        callback(ppapi.PP_Error.PP_ERROR_FAILED);
      }
    };

    file_system.fs.root.getFile(ref.path, {}, function(entry) {
      entry.remove(function() {
        callback(ppapi.PP_Error.PP_OK);
      }, DummyError);
    }, error_handler);

    return ppapi.PP_Error.PP_OK_COMPLETIONPENDING;
  };

  var FileRef_Rename = function() {
    throw "FileRef_Rename not implemented";
  };

  registerInterface("PPB_FileRef;1.0", [
      FileRef_Create,
      FileRef_IsFileRef,
      FileRef_GetFileSystemType,
      FileRef_GetName,
      FileRef_GetPath,
      FileRef_GetParent,
      FileRef_MakeDirectory,
      FileRef_Touch,
      FileRef_Delete,
      FileRef_Rename
  ]);

})();
