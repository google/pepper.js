(function() {

  var MouseLock_LockMouse = function(instance, callback) {
    var canvas = resources.resolve(instance).canvas;
    var cb_func = ppapi_glue.convertCompletionCallback(callback);

    var makeCallback = function(return_code) {
      return function(event) {
        cb_func(return_code);
        return true;
      };
    };

    if('webkitRequestPointerLock' in canvas) {
      // TODO(grosse): Figure out how to handle the callbacks properly
      canvas.addEventListener('webkitpointerlockchange', makeCallback(ppapi.PP_Error.PP_OK));
      canvas.addEventListener('webkitpointerlockerror', makeCallback(ppapi.PP_Error.PP_ERROR_FAILED));
      canvas.webkitRequestPointerLock();
    } else {
      // Note: This may not work as Firefox currently requires fullscreen before requesting pointer lock
      canvas.addEventListener('mozpointerlockchange', makeCallback(ppapi.PP_Error.PP_OK));
      canvas.addEventListener('mozpointerlockerror', makeCallback(ppapi.PP_Error.PP_ERROR_FAILED));
      canvas.mozRequestPointerLock();
    }
  };

  var MouseLock_UnlockMouse = function(instance) {
    throw "MouseLock_UnlockMouse not implemented";
  };


  registerInterface("PPB_MouseLock;1.0", [
    MouseLock_LockMouse,
    MouseLock_UnlockMouse,
  ]);

  var FullScreen_IsFullscreen = function(instance) {
    var element = document.fullscreenElement || document.webkitFullscreenElement || document.mozFullScreenElement;
    return element === resources.resolve(instance).canvas;
  };

  var FullScreen_SetFullscreen = function(instance, fullscreen) {
    var resource = resources.resolve(instance)
    var element = resource.canvas;

    if (fullscreen) {
      if (element.requestFullscreen) {
        element.requestFullscreen();
      } else if (element.mozRequestFullScreen) {
        element.mozRequestFullScreen();
      } else if (element.webkitRequestFullscreen) {
        element.webkitRequestFullscreen();
      }
    } else {
      throw "FullScreen_SetFullscreen(..., false) not supported";
    }

    return true;
  };


  var FullScreen_GetScreenSize = function() {
    throw "FullScreen_GetScreenSize not implemented";
  };

  registerInterface("PPB_Fullscreen;1.0", [
    FullScreen_IsFullscreen,
    FullScreen_SetFullscreen,
    FullScreen_GetScreenSize
  ]);

})();
