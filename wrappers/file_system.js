(function() {

  var FILE_SYSTEM_RESOURCE = "filesystem";

  var PP_FILESYSTEMTYPE_INVALID = 0;
  var PP_FILESYSTEMTYPE_EXTERNAL = 1;
  var PP_FILESYSTEMTYPE_LOCALPERSISTENT = 2;
  var PP_FILESYSTEMTYPE_LOCALTEMPORARY = 3;

  var fsTypeMap = {};
  fsTypeMap[PP_FILESYSTEMTYPE_LOCALPERSISTENT] = window.PERSISTENT;
  fsTypeMap[PP_FILESYSTEMTYPE_LOCALTEMPORARY] = window.TEMPORARY;

  var FileSystem_Create = function(instance, type) {
    // Creating a filesystem is asynchronous, so just store args for later
    return resources.register(FILE_SYSTEM_RESOURCE, {fs_type: type, fs: null});
  };

  var FileSystem_IsFileSystem = function(res) {
    return resources.is(res, FILE_SYSTEM_RESOURCE);
  };

  // Note that int64s are passed as two arguments, with high word second
  var FileSystem_Open = function(file_system, size_low, size_high, callback_ptr) {
    var res = resources.resolve(file_system, FILE_SYSTEM_RESOURCE);
    if (res === undefined) {
      return ppapi.PP_Error.PP_ERROR_BADRESOURCE;
    }
    var callback = ppapi_glue.convertCompletionCallback(callback_ptr);

    var type = fsTypeMap[res.fs_type];
    if (type === undefined) {
      return ppapi.PP_Error.PP_ERROR_FAILED;
    }

    var requestFS = window.requestFileSystem || window.webkitRequestFileSystem;
    requestFS(type, util.ToI64(size_low, size_high), function(fs) {
      res.fs = fs;
      callback(ppapi.PP_Error.PP_OK);
    }, function(error) {
      console.log('Error!', error);
      callback(ppapi.PP_Error.PP_ERROR_FAILED);
    });

    return ppapi.PP_Error.PP_OK_COMPLETIONPENDING;
  };

  var FileSystem_GetType = function() {
    throw "FileSystem_GetType not implemented";
  };


  registerInterface("PPB_FileSystem;1.0", [
      FileSystem_Create,
      FileSystem_IsFileSystem,
      FileSystem_Open,
      FileSystem_GetType
  ]);

})();
