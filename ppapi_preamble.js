// TODO(ncbray): re-enable once Emscripten stops including code with octal values.
//"use strict";

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

var getFullscreenElement = function() {
  return document.fullscreenElement || document.webkitFullscreenElement || document.mozFullScreenElement || null;
}

var clamp = function(value, min, max) {
  if (value < min) {
    return min;
  } else if (value > max) {
    return max;
  } else {
    return value;
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

// Encoding types as numbers instead of string saves ~2kB when minified because closure will inline these constants.
var STRING_RESOURCE = 0;
var ARRAY_BUFFER_RESOURCE = 1;

var INPUT_EVENT_RESOURCE = 2;

var FILE_SYSTEM_RESOURCE = 3;
var FILE_REF_RESOURCE = 4;
var FILE_IO_RESOURCE = 5;

var URL_LOADER_RESOURCE = 6;
var URL_REQUEST_INFO_RESOURCE = 7;
var URL_RESPONSE_INFO_RESOURCE = 8;

var AUDIO_CONFIG_RESOURCE = 9;
var AUDIO_RESOURCE = 10;

var GRAPHICS_2D_RESOURCE = 11;
var IMAGE_DATA_RESOURCE = 12;

var PROGRAM_RESOURCE = 13;
var SHADER_RESOURCE = 14;
var BUFFER_RESOURCE = 15;
var TEXTURE_RESOURCE = 16;
var UNIFORM_LOCATION_RESOURCE = 17;

var INSTANCE_RESOURCE = 18;
var VIEW_RESOURCE = 19;
var GRAPHICS_3D_RESOURCE = 20;

var ResourceManager = function() {
  this.lut = {};
  this.uid = 1;
  this.num_resources = 0;
}

ResourceManager.prototype.checkType = function(type) {
  if (typeof type !== "number") {
    throw "resource type must be a number";
  }
}

ResourceManager.prototype.register = function(type, res) {
  this.checkType(type);
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

var uidInfo = function(uid, type) {
  return "(" + uid + " as " + type + ")";
}

ResourceManager.prototype.resolve = function(uid, type, speculative) {
  if (typeof uid !== "number") {
    throw "resources.resolve uid must be an int";
  }
  this.checkType(type);
  if (uid === 0) {
    if (!speculative) {
      console.error("Attempted to resolve an invalid resource ID " + uidInfo(uid, type));
    }
    return undefined;
  }
  var res = this.lut[uid];
  if (res === undefined) {
    console.error("Attempted to resolve a non-existant resource ID " + uidInfo(uid, type));
    return undefined;
  }
  if (res.type !== type) {
    if (!speculative) {
      console.error("Expected resource " + uidInfo(uid, type) + ", but it was " + uidInfo(uid, res.type));
    }
    return undefined;
  }
  return res;
}

ResourceManager.prototype.is = function(uid, type) {
  return this.resolve(uid, type, true) !== undefined;
}

ResourceManager.prototype.addRef = function(uid) {
  var res = this.lut[uid];
  if (res === undefined) {
    throw "Resource does not exist: " + uid;
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
var declaredInterfaces = [];

var registerInterface = function(name, functions, supported) {
  // Defer creating the interface until Emscripten's runtime is available.
  declaredInterfaces.push({name: name, functions: functions, supported: supported});
};

var createInterface = function(name, functions) {
  var trace = function(f, i) {
    return function() {
      console.log(">>>", name, i, arguments);
      var result = f.apply(f, arguments);
      console.log("<<<", name, i, result);
      return result;
    }
  };

  var getFuncPtr = function(f) {
    // Memoize - a single function may appear in multiple versions of an interface.
    if (f.func_ptr === undefined) {
      f.func_ptr = Runtime.addFunction(f, 1);
    }
    return f.func_ptr;
  };

  // allocate(...) is bugged for non-i8 allocations, so do it manually
  // TODO(ncbray): static alloc?
  var ptr = allocate(functions.length * 4, 'i8', ALLOC_NORMAL);
  for (var i in functions) {
    var f = functions[i];
    if (false) {
      f = trace(f, i);
    }
    setValue(ptr + i * 4, getFuncPtr(f), 'i32');
  }
  interfaces[name] = ptr;
};

var Module = {
  "noInitialRun": true,
  "noExitRuntime": true,
  "preInit": function() {
    for (var i = 0; i < declaredInterfaces.length; i++) {
      var inf = declaredInterfaces[i];
      if (inf.supported === undefined || inf.supported()) {
        createInterface(inf.name, inf.functions);
      } else {
        interfaces[inf.name] = 0;
      }
    }
    declaredInterfaces = [];
  }
};

var CreateInstance = function(width, height, shadow_instance) {
  if (shadow_instance === undefined) {
    shadow_instance = document.createElement("span");
    shadow_instance.setAttribute("name", "nacl_module");
    shadow_instance.setAttribute("id", "nacl_module");
  }

  shadow_instance.setAttribute("width", width);
  shadow_instance.setAttribute("height", height);
  shadow_instance.className = "ppapiJsEmbed";

  shadow_instance.style.display = "inline-block";
  shadow_instance.style.width = width + "px";
  shadow_instance.style.height = height + "px";
  shadow_instance.style.overflow = "hidden";

  // Called from external code.
  shadow_instance["postMessage"] = postMessage;

  // Not compatible with CSP.
  var style = document.createElement("style");
  style.type = "text/css";
  style.innerHTML = ".ppapiJsEmbed {border: 0px; margin: 0px; padding: 0px;}";
  style.innerHTML += " .ppapiJsCanvas {image-rendering: optimizeSpeed; image-rendering: -moz-crisp-edges; image-rendering: -o-crisp-edges; image-rendering: -webkit-optimize-contrast; image-rendering: optimize-contrast; -ms-interpolation-mode: nearest-neighbor;}";
  // Bug-ish.  Each variation needs to be specified seperately.
  var fullscreenCSS = "{position: fixed; top: 0; left: 0; bottom: 0; right: 0; width: 100% !important; height: 100% !important; box-sizing: border-box; object-fit: contain; background-color: black;}";
  style.innerHTML += " .ppapiJsEmbed:-webkit-full-screen " + fullscreenCSS;
  style.innerHTML += " .ppapiJsEmbed:-moz-full-screen " + fullscreenCSS;
  style.innerHTML += " .ppapiJsEmbed:full-screen " + fullscreenCSS;

  document.getElementsByTagName("head")[0].appendChild(style);

  var canvas = document.createElement('canvas');
  canvas.className = "ppapiJsCanvas";
  canvas.width = width;
  canvas.height = height;
  canvas.style.border = "0px";
  canvas.style.padding = "0px";
  canvas.style.margin = "0px";
  canvas.onselectstart = function(evt) {
    evt.preventDefault();
    return false;
  };
  shadow_instance.appendChild(canvas);

  var last_update = "";
  var updateView = function() {
    // NOTE: this will give the wrong value if the canvas has any margin, border, or padding.
    var bounds = shadow_instance.getBoundingClientRect();
    var rect = {x: bounds.left, y: bounds.top,
                width: bounds.right - bounds.left,
                height: bounds.bottom - bounds.top};

    // Clip the bounds to the viewport.
    var clipX = clamp(bounds.left, 0, window.innerWidth);
    var clipY = clamp(bounds.top, 0, window.innerHeight);
    var clipWidth = clamp(bounds.right, 0, window.innerWidth) - clipX;
    var clipHeight = clamp(bounds.bottom, 0, window.innerHeight) - clipY;

    // Translate into the coordinate space of the canvas.
    clipX -= bounds.left;
    clipY -= bounds.top;

    // Handle a zero-sized clip region.
    var visible = clipWidth > 0 && clipHeight > 0;
    if (!visible) {
      // clipX and clipY may be outside the canvas if width or height are zero.
      // The PPAPI spec requires we return (0, 0, 0, 0)
      clipX = 0;
      clipY = 0;
      clipWidth = 0;
      clipHeight = 0;
    }
    var event = {
      rect: rect,
      fullscreen: getFullscreenElement() === shadow_instance,
      visible: visible,
      page_visible: 1,
      clip_rect: {x: clipX, y: clipY, width: clipWidth, height: clipHeight}
    };
    var s = JSON.stringify(event);
    if (s !== last_update) {
      last_update = s;
      var view = resources.register(VIEW_RESOURCE, event);
      _DoChangeView(instance, view);
      resources.release(view);
    }
  };

  var instance = resources.register(INSTANCE_RESOURCE, {
    element: shadow_instance,
    device: null,
    canvas: canvas,
    bind: function(device) {
      this.unbind();
      this.device = device;
      resources.addRef(this.device.uid);
      this.device.notifyBound(this);
    },
    unbind: function() {
      if (this.device) {
        this.device.notifyUnbound(this);
        resources.release(this.device.uid);
        this.device = null;
      }
    },
    destroy: function() {
      this.unbind();
    }
  });

  // Called from external code.
  shadow_instance["finishLoading"] = function() {
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

    updateView();

    var sendProgressEvent = function(name) {
      var evt = document.createEvent('Event');
      evt.initEvent(name, true, true);  // bubbles, cancelable
      shadow_instance.dispatchEvent(evt);
    }

    // Fake the load sequence.
    shadow_instance.readyState = 4;
    sendProgressEvent('load');
    sendProgressEvent('loadend');
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

  window.addEventListener('DOMContentLoaded', updateView);
  window.addEventListener('load', updateView);
  window.addEventListener('scroll', updateView);
  window.addEventListener('resize', updateView);

  // TODO(ncbray): element resize.

  // TODO(grosse): Make this work when creating multiple instances or modules.
  // It should only be called once when the page loads.
  // Currently it's called everytime an instance is created.
  var fullscreenChange = function() {
    var doSend = function(entering_fullscreen, target) {
      var instance_id = target.instance;
      var inst = resources.resolve(instance_id, INSTANCE_RESOURCE);
      if (inst === undefined) {
        return;
      }
    };

    setTimeout(updateView, 1);

    // Keep track of current fullscreen element
    var lastTarget = null;
    return function(event) {
      // When an event occurs because fullscreen is lost, the element will be null but we have no direct way of determining
      // which element lost fullscreen. So we keep track of the previous element ot enter fullscreen.
      var target = getFullscreenElement();
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

  var target = shadow_instance;
  if (target.requestFullscreen) {
    document.addEventListener('fullscreenchange', fullscreenChange);
  } else if (target.mozRequestFullScreen) {
    document.addEventListener('mozfullscreenchange', fullscreenChange);
  } else if (target.webkitRequestFullscreen) {
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
  var ppapi = {
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