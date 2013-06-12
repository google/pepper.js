(function() {

  var PP_FLAGS_WRITE = 1 << 1;
  var PP_FLAGS_CREATE = 1 << 2;
  var PP_FLAGS_TRUNCATE = 1 << 3;
  var PP_FLAGS_EXCLUSIVE = 1 << 4;

  var PP_FILETYPE_REGULAR = 0;
  var PP_FILETYPE_DIRECTORY = 1;
  var PP_FILETYPE_OTHER = 2;


  var DummyError = function(error) {
    console.log('Unhandled fileio error!', error);
    throw 'Unhandled fileio error: ' + error;
  };

  var FileIO_Create = function(instance) {
    if (!resources.is(instance, 'instance')) {
      return 0;
    }
    return resources.register('fileio', {
        closed: false,
        entry: null,
        fs_type: 0,
        flags: 0
    });
  };

  var FileIO_IsFileIO = function() {
    throw "FileIO_IsFileIO not implemented";
  };

  var FileIO_Open = function(file_io, file_ref, flags, callback_ptr) {
    var io = resources.resolve(file_io);
    var ref = resources.resolve(file_ref);
    var file_system = resources.resolve(ref.file_system);
    var callback = ppapi_glue.convertCompletionCallback(callback_ptr);

    if (io.closed || io.entry !== null) {
      return ppapi.PP_Error.PP_ERROR_FAILED;
    }

    var js_flags = {
      create: (flags & PP_FLAGS_CREATE) !== 0,
      exclusive: (flags & PP_FLAGS_EXCLUSIVE) !== 0
    };

    file_system.fs.root.getFile(ref.path, js_flags, function(entry) {
      if (io.dead || file_system.dead) {
        return callback(ppapi.PP_Error.PP_ERROR_ABORTED);
      }

      io.entry = entry;
      io.fs_type = file_system.fs_type;

      if (flags & PP_FLAGS_TRUNCATE) {
        entry.createWriter(function(writer) {
          writer.onwrite = function(event) {
            callback(ppapi.PP_Error.PP_OK);
          }
          writer.onerror = DummyError;
          writer.truncate(0);
        }, DummyError);
      } else {
        callback(ppapi.PP_Error.PP_OK);
      }
    }, function(error) {
      var code = error.code;
      if (code === FileError.NOT_FOUND_ERR) {
        callback(ppapi.PP_Error.PP_ERROR_FILENOTFOUND)
      } else {
        callback(ppapi.PP_Error.PP_ERROR_FAILED)
      }
    });
    return ppapi.PP_Error.PP_OK_COMPLETIONPENDING;
  };

  var AccessFile = function(file_io, callback_ptr, body) {
    var io = resources.resolve(file_io);
    var callback = ppapi_glue.convertCompletionCallback(callback_ptr);
    if (io.closed) {
      return callback(ppapi.PP_Error.PP_ERROR_ABORTED);
    }

    body(io, io.entry, callback);
    return ppapi.PP_Error.PP_OK_COMPLETIONPENDING;
  }

  var FileIO_Query = function(file_io, info_ptr, callback_ptr) {
    return AccessFile(file_io, callback_ptr, function(io, entry, callback) {
        entry.getMetadata(function(metadata) {

          if (io.dead) {
            return callback(ppapi.PP_Error.PP_ERROR_ABORTED);
          }

          var info = {
              size_low: metadata.size % util.k2_32,
              size_high: (metadata.size / util.k2_32) | 0,
              type: PP_FILETYPE_REGULAR,
              system_type: io.fs_type,
              creation_time: 0.0,
              last_access_time: 0.0,
              last_modified_time: metadata.modificationTime ? metadata.modificationTime.valueOf() / 1000 : 0.0
          };

          ppapi_glue.setFileInfo(info, info_ptr);
          callback(ppapi.PP_Error.PP_OK);
        }, DummyError);
    });
  };

  var FileIO_Touch = function() {
    throw "FileIO_Touch not implemented";
  };

  var FileIO_Read = function(file_io, offset_low, offset_high, output_ptr, bytes_to_read, callback_ptr) {
    return AccessFile(file_io, callback_ptr, function(io, entry, callback) {

        entry.file(function(file) {

          var reader = new FileReader();
          reader.onprogress = function(event) {

            // Make sure this is the final progress event (everything's loaded)
            if (reader.readyState !== reader.DONE) {
              return;
            }

            var offset = util.ToI64(offset_low, offset_high);
            var buffer = reader.result.slice(offset, offset + bytes_to_read);
            HEAP8.set(new Int8Array(buffer), output_ptr);
            callback(buffer.byteLength);
          };
          reader.onerror = DummyError;
          reader.readAsArrayBuffer(file);
        }, DummyError);
    });
  };

  var FileIO_Write = function(file_io, offset_low, offset_high, input_ptr, bytes_to_read, callback_ptr) {
    return AccessFile(file_io, callback_ptr, function(io, entry, callback) {
        entry.createWriter(function(writer) {
          var buffer = HEAP8.subarray(input_ptr, input_ptr + bytes_to_read);
          var offset = util.ToI64(offset_low, offset_high);

          writer.seek(offset);
          writer.onprogress = function(event) {
            // Make sure this is the final progress event (everything's loaded)
            if (writer.readyState !== writer.DONE) {
              return;
            }

            callback(buffer.byteLength);
          }
          writer.onerror = DummyError;
          writer.write(new Blob([buffer]));
        }, DummyError);
    });
  };

  var FileIO_SetLength = function() {
    throw "FileIO_SetLength not implemented";
  };

  var FileIO_Flush = function(file_io, callback_ptr) {
    // Basically a NOP in the current implementation
    var io = resources.resolve(file_io);
    var callback = ppapi_glue.convertCompletionCallback(callback_ptr);
    callback(io.closed ? ppapi.PP_Error.PP_ERROR_ABORTED : ppapi.PP_Error.PP_OK);
  };

  var FileIO_Close = function() {
    throw "FileIO_Close not implemented";
  };

  var FileIO_ReadToArray = function() {
    throw "FileIO_ReadToArray not implemented";
  };


  registerInterface("PPB_FileIO;1.1", [
      FileIO_Create,
      FileIO_IsFileIO,
      FileIO_Open,
      FileIO_Query,
      FileIO_Touch,
      FileIO_Read,
      FileIO_Write,
      FileIO_SetLength,
      FileIO_Flush,
      FileIO_Close,
      FileIO_ReadToArray
  ]);

  registerInterface("PPB_FileIO;1.0", [
      FileIO_Create,
      FileIO_IsFileIO,
      FileIO_Open,
      FileIO_Query,
      FileIO_Touch,
      FileIO_Read,
      FileIO_Write,
      FileIO_SetLength,
      FileIO_Flush,
      FileIO_Close
  ]);

})();
