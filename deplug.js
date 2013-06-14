"use strict";

// Polyfill for Safari.
if (window.performance === undefined) {
  window.performance = {};
}
if (window.performance.now === undefined) {
  var nowStart = Date.now();
  window.performance.now = function() {
    return Date.now() - nowStart;
  }
}

var postMessage = function(message) {

  // Fill out the PP_Var structure
  var o = ppapi_glue.PP_Var
  var var_ptr = _malloc(o.__size__);
  ppapi_glue.varForJS(var_ptr, message);

  // Send
  _DoPostMessage(this.instance, var_ptr);

  // Cleanup
  // Note: the callee releases the var so we don't need to.
  // This is different than most interfaces.
  _free(var_ptr);
}

var STRING_RESOURCE = "string";
var ARRAY_BUFFER_RESOURCE = "array_buffer";

var ResourceManager = function() {
  this.lut = {};
  this.uid = 1;
  this.num_resources = 0;
}

ResourceManager.prototype.register = function(type, res) {
  if (typeof type !== "string") {
    throw "resources.register type must be a string";
  }
  while (this.uid in this.lut || this.uid === 0) {
    this.uid = (this.uid + 1) & 0xffffffff;
  }
  res.type = type;
  res.uid = this.uid;
  res.refcount = 1;
  this.lut[res.uid] = res;
  this.num_resources += 1;
  this.dead = false;
  return this.uid;
}

ResourceManager.prototype.registerString = function(value, memory, len) {
  return this.register(STRING_RESOURCE, {
      value: value,
      memory: memory,
      len: len,
      destroy: function() {
	_free(this.memory);
      }
  });
}

ResourceManager.prototype.registerArrayBuffer = function(memory, len) {
  return this.register(ARRAY_BUFFER_RESOURCE, {
      memory: memory,
      len: len,
      destroy: function() {
	_free(this.memory);
      }
  });
}

ResourceManager.prototype.resolve = function(uid, type) {
  if (typeof uid !== "number") {
    throw "resources.resolve uid must be an int";
  }
  if (type !== undefined && typeof type !== "string") {
    throw "resources.resolve type must be a string";
  }
  if (uid === 0) {
    console.warn("Attempted to resolve an invalid resource ID (0/" + type + ")");
    return undefined;
  }
  var res = this.lut[uid];
  if (res === undefined) {
    console.warn("Attempted to resolve a non-existant resource ID (" + uid + "/" + type + ")");
    return undefined;
  }
  if (type !== undefined && res.type !== type) {
    console.warn("Expected a resource of type " + type + ", but got one of type " + res.type);
    return undefined;
  }
  return res;
}

ResourceManager.prototype.is = function(uid, type) {
  return this.resolve(uid, type) !== undefined;
}

ResourceManager.prototype.addRef = function(uid) {
  var res = this.lut[uid];
  if (res === undefined) {
    throw "Resource does not exist.";
  }
  res.refcount += 1;
}

ResourceManager.prototype.release = function(uid) {
  var res = this.lut[uid];
  if (res === undefined) {
    throw "Resource does not exist: " + uid;
  }

  res.refcount -= 1;
  if (res.refcount <= 0) {
    if (res.destroy) {
      res.destroy();
    }

    res.dead = true;
    delete this.lut[res.uid];
    this.num_resources -= 1;
  }
}

ResourceManager.prototype.getNumResources = function() {
  return this.num_resources;
}

var resources = new ResourceManager();
var interfaces = {};

var registerInterface = function(name, functions) {
  var trace = function(f, i) {
    return function() {
      console.log(">>>", name, i, arguments);
      var result = f.apply(f, arguments);
      console.log("<<<", name, i, result);
      return result;
    }
  }

  // allocate(...) is bugged for non-i8 allocations, so do it manually
  // TODO(ncbray): static alloc?
  var ptr = allocate(functions.length * 4, 'i8', ALLOC_NORMAL);
  for (var i in functions) {
    var f = functions[i];
    if (false) {
      f = trace(f, i);
    }
    setValue(ptr + i * 4, Runtime.addFunction(f, 1), 'i32');
  }
  interfaces[name] = ptr;
};

var Module = {};

var INSTANCE_RESOURCE = "instance";
var VIEW_RESOURCE = "view";
var GRAPHICS_3D_RESOURCE = "graphics_3d";

var CreateInstance = function(width, height, shadow_instance) {
  if (shadow_instance === undefined) {
    shadow_instance = document.createElement("span");
    shadow_instance.setAttribute("name", "nacl_module");
    shadow_instance.setAttribute("id", "nacl_module");
  }

  shadow_instance.setAttribute("width", width);
  shadow_instance.setAttribute("height", height);

  shadow_instance.style.display = "inline-block";
  shadow_instance.style.width = width + "px";
  shadow_instance.style.height = height + "px";
  shadow_instance.style.padding = "0px";

  // Called from external code.
  shadow_instance["postMessage"] = postMessage;

  var canvas = document.createElement('canvas');
  canvas.width = width;
  canvas.height = height;
  shadow_instance.appendChild(canvas);

  var sendViewEvent = function(instance_id, view_obj) {
    var view = resources.register(VIEW_RESOURCE, view_obj);
    _DoChangeView(instance_id, view);
    resources.release(view);
  };

  // Called from external code.
  shadow_instance["finishLoading"] = function() {
    var instance = resources.register(INSTANCE_RESOURCE, {
      element: shadow_instance,
      canvas: canvas,
      // Save original size so it can be restored later
      size: {
            width: canvas.width,
            height: canvas.height
        }
    });

    // Allows shadow_instance.postMessage to work.
    // This is only a UID so there is no circular reference.
    shadow_instance.instance = instance;

    // Turn the element's attributes into PPAPI's arguments.
    // TODO(ncbray): filter out style attribute?
    var argc = shadow_instance.attributes.length;
    var argn = allocate(argc * 4, 'i8', ALLOC_NORMAL);
    var argv = allocate(argc * 4, 'i8', ALLOC_NORMAL);
    for (var i = 0; i < argc; i++) {
      var attribute = shadow_instance.attributes[i];
      setValue(argn + i * 4, allocate(intArrayFromString(attribute.name), 'i8', ALLOC_NORMAL), 'i32');
      setValue(argv + i * 4, allocate(intArrayFromString(attribute.value), 'i8', ALLOC_NORMAL), 'i32');
    }

    _NativeCreateInstance(instance, argc, argn, argv);

    // Clean up the arguments.
    for (var i = 0; i < argc; i++) {
      _free(getValue(argn + i * 4, 'i32'));
      _free(getValue(argv + i * 4, 'i32'));
    }
    _free(argn);
    _free(argv);

    // Create and send a bogus view resource.
    sendViewEvent(instance, {
      rect: {x: 0, y: 0, width: width, height: height},
      fullscreen: 0,
      visible: 1,
      page_visible: 1,
      clip_rect: {x: 0, y: 0, width: width, height: height}
    });

    // Fake the load event.
    var evt = document.createEvent('Event');
    evt.initEvent('load', true, true);  // bubbles, cancelable
    shadow_instance.dispatchEvent(evt);
  };

  var makeCallback = function(hasFocus){
    return function(event) {
      _DoChangeFocus(shadow_instance.instance, hasFocus);
      return true;
    };
  };

  canvas.setAttribute('tabindex', '0'); // make it focusable
  canvas.addEventListener('focus', makeCallback(true));
  canvas.addEventListener('blur', makeCallback(false));


  // TODO(grosse): Make this work when creating multiple instances or modules.
  // It should only be called once when the page loads.
  // Currently it's called everytime an instance is created.
  var fullscreenChange = function() {
    var doSend = function(entering_fullscreen, canvas) {
      var instance_id = canvas.parentElement.instance;
      var inst = resources.resolve(instance_id, INSTANCE_RESOURCE);
      if (inst === undefined) {
	return;
      }
      var origsize = inst.size;

      var width = entering_fullscreen ? window.screen.width : origsize.width;
      var height = entering_fullscreen ? window.screen.height : origsize.height;

      sendViewEvent(instance_id, {
        rect: {x: 0, y: 0, width: width, height: height},
        fullscreen: entering_fullscreen ? 1 : 0,
        visible: 1,
        page_visible: 1,
        clip_rect: {x: 0, y: 0, width: width, height: height}
      });

      // Chrome doesn't currently add the required CSS for fullscreen, so we have to add it manually
      // which means removing it when leaving fullscreen
      if (!entering_fullscreen && canvas.webkitRequestFullscreen && !canvas.requestFullscreen) {
        var style = canvas.style;
        for (var key in style) {
          if (style.hasOwnProperty(key)) {
            style[key] = null;
          }
        }
      }
    };

    // Keep track of current fullscreen element
    var lastTarget = null;
    return function(event) {
      // When an event occurs because fullscreen is lost, the element will be null but we have no direct way of determining
      // which element lost fullscreen. So we keep track of the previous element ot enter fullscreen.
      var target = document.fullscreenElement || document.webkitFullscreenElement || document.mozFullScreenElement || null;
      var entering_fullscreen = (target !== null);

      if (target !== lastTarget) {
        // Send a fullscreen lost event to previous element if any
        if (lastTarget !== null) {
          doSend(false, lastTarget);
          lastTarget = null;
        }

        if (target !== null) {
          doSend(true, target);
        }
        lastTarget = target;
      }
    };
  }();

  if (canvas.requestFullscreen) {
    document.addEventListener('fullscreenchange', fullscreenChange);
  } else if (canvas.mozRequestFullScreen) {
    document.addEventListener('mozfullscreenchange', fullscreenChange);
  } else if (canvas.webkitRequestFullscreen) {
    document.addEventListener('webkitfullscreenchange', fullscreenChange);
  }

  // TODO handle removal events.
  return shadow_instance;
}

// Entry point
window["CreateInstance"] = CreateInstance;

var util = (function() {
  return {

    k2_32: 0x100000000,
    ToI64: function(low, high){
      var val = low + (high * util.k2_32);
      if (((val - low) / util.k2_32) !== high || (val % util.k2_32) !== low) {
        throw "Value " + String([low,high]) + " cannot be represented as a Javascript number";
      }

      return val;
    },

    decodeUTF8: function(ptr, len) {
      var chars = [];
      var i = 0;
      var val;
      var n;
      var b;

      // If no len is provided, assume null termination
      while (len === undefined || i < len) {
        b = HEAPU8[ptr + i];
        if (len === undefined && b === 0) {
          break;
        }

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
    }
  };
})();


var ppapi = (function() {
  var ppapi = {};

  ppapi.PP_Error = {
    /**
     * This value is returned by a function on successful synchronous completion
     * or is passed as a result to a PP_CompletionCallback_Func on successful
     * asynchronous completion.
     */
    PP_OK: 0,
    /**
     * This value is returned by a function that accepts a PP_CompletionCallback
     * and cannot complete synchronously. This code indicates that the given
     * callback will be asynchronously notified of the final result once it is
     * available.
     */
    PP_OK_COMPLETIONPENDING: -1,
    /**This value indicates failure for unspecified reasons. */
    PP_ERROR_FAILED: -2,
    /**
     * This value indicates failure due to an asynchronous operation being
     * interrupted. The most common cause of this error code is destroying a
     * resource that still has a callback pending. All callbacks are guaranteed
     * to execute, so any callbacks pending on a destroyed resource will be
     * issued with PP_ERROR_ABORTED.
     *
     * If you get an aborted notification that you aren't expecting, check to
     * make sure that the resource you're using is still in scope. A common
     * mistake is to create a resource on the stack, which will destroy the
     * resource as soon as the function returns.
     */
    PP_ERROR_ABORTED: -3,
    /** This value indicates failure due to an invalid argument. */
    PP_ERROR_BADARGUMENT: -4,
    /** This value indicates failure due to an invalid PP_Resource. */
    PP_ERROR_BADRESOURCE: -5,
    /** This value indicates failure due to an unavailable PPAPI interface. */
    PP_ERROR_NOINTERFACE: -6,
    /** This value indicates failure due to insufficient privileges. */
    PP_ERROR_NOACCESS: -7,
    /** This value indicates failure due to insufficient memory. */
    PP_ERROR_NOMEMORY: -8,
    /** This value indicates failure due to insufficient storage space. */
    PP_ERROR_NOSPACE: -9,
    /** This value indicates failure due to insufficient storage quota. */
    PP_ERROR_NOQUOTA: -10,
    /**
     * This value indicates failure due to an action already being in
     * progress.
     */
    PP_ERROR_INPROGRESS: -11,
    /** This value indicates failure due to a file that does not exist. */
    /**
     * The requested command is not supported by the browser.
     */
    PP_ERROR_NOTSUPPORTED: -12,
    /**
     * Returned if you try to use a null completion callback to "block until
     * complete" on the main thread. Blocking the main thread is not permitted
     * to keep the browser responsive (otherwise, you may not be able to handle
     * input events, and there are reentrancy and deadlock issues).
     *
     * The goal is to provide blocking calls from background threads, but PPAPI
     * calls on background threads are not currently supported. Until this
     * support is complete, you must either do asynchronous operations on the
     * main thread, or provide an adaptor for a blocking background thread to
     * execute the operaitions on the main thread.
     */
    PP_ERROR_BLOCKS_MAIN_THREAD: -13,
    PP_ERROR_FILENOTFOUND: -20,
    /** This value indicates failure due to a file that already exists. */
    PP_ERROR_FILEEXISTS: -21,
    /** This value indicates failure due to a file that is too big. */
    PP_ERROR_FILETOOBIG: -22,
    /**
     * This value indicates failure due to a file having been modified
     * unexpectedly.
     */
    PP_ERROR_FILECHANGED: -23,
    /** This value indicates failure due to a time limit being exceeded. */
    PP_ERROR_TIMEDOUT: -30,
    /**
     * This value indicates that the user cancelled rather than providing
     * expected input.
     */
    PP_ERROR_USERCANCEL: -40,
    /**
     * This value indicates failure due to lack of a user gesture such as a
     * mouse click or key input event. Examples of actions requiring a user
     * gesture are showing the file chooser dialog and going into fullscreen
     * mode.
     */
    PP_ERROR_NO_USER_GESTURE: -41,
    /**
     * This value indicates that the graphics context was lost due to a
     * power management event.
     */
    PP_ERROR_CONTEXT_LOST: -50,
    /**
     * Indicates an attempt to make a PPAPI call on a thread without previously
     * registering a message loop via PPB_MessageLoop.AttachToCurrentThread.
     * Without this registration step, no PPAPI calls are supported.
     */
    PP_ERROR_NO_MESSAGE_LOOP: -51,
    /**
     * Indicates that the requested operation is not permitted on the current
     * thread.
     */
    PP_ERROR_WRONG_THREAD: -52
  };

  return ppapi;
})();