// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Javascript module pattern:
//   see http://en.wikipedia.org/wiki/Unobtrusive_JavaScript#Namespaces
// In essence, we define an anonymous function which is immediately called and
// returns a new object. The new object contains only the exported definitions;
// all other definitions in the anonymous function are inaccessible to external
// code.
var common = (function () {

  function createEmscriptenModule(name, tool, path, width, height) {
    var moduleEl = document.createElement('embed');
    moduleEl.setAttribute('name', 'nacl_module');
    moduleEl.setAttribute('id', 'nacl_module');
    moduleEl.setAttribute('width', width);
    moduleEl.setAttribute('height',height);

    moduleEl.postMessage = ppapi.postMessage;

    var listenerDiv = document.getElementById('listener');
    listenerDiv.appendChild(moduleEl);

    // HACK global
    fakeEmbed = moduleEl;

    var instance = Module._Startup();

    // Fake the load event.
    var evt = document.createEvent('Event');
    evt.initEvent('load', true, true);  // bubbles, cancelable
    moduleEl.dispatchEvent(evt);
  }

  /**
   * Create the Native Client <embed> element as a child of the DOM element
   * named "listener".
   *
   * @param {string} name The name of the example.
   * @param {string} tool The name of the toolchain, e.g. "glibc", "newlib" etc.
   * @param {string} path Directory name where .nmf file can be found.
   * @param {number} width The width to create the plugin.
   * @param {number} height The height to create the plugin.
   */
  function createNaClModule(name, tool, path, width, height) {
    if (tool == 'emscripten') {
      return createEmscriptenModule(name, tool, path, width, height);
    }
    var moduleEl = document.createElement('embed');
    moduleEl.setAttribute('name', 'nacl_module');
    moduleEl.setAttribute('id', 'nacl_module');
    moduleEl.setAttribute('width', width);
    moduleEl.setAttribute('height',height);
    moduleEl.setAttribute('src', path + '/' + name + '.nmf');
    // For NaCL modules use application/x-nacl.
    var mimetype = 'application/x-nacl';
    var isHost = tool == 'win' || tool == 'linux' || tool == 'mac';
    if (isHost) {
      // For non-nacl PPAPI plugins use the x-ppapi-debug/release
      // mime type.
      if (path.toLowerCase().indexOf('release') != -1)
        mimetype = 'application/x-ppapi-release';
      else
        mimetype = 'application/x-ppapi-debug';
    }
    moduleEl.setAttribute('type', mimetype);

    // The <EMBED> element is wrapped inside a <DIV>, which has both a 'load'
    // and a 'message' event listener attached.  This wrapping method is used
    // instead of attaching the event listeners directly to the <EMBED> element
    // to ensure that the listeners are active before the NaCl module 'load'
    // event fires.
    var listenerDiv = document.getElementById('listener');
    listenerDiv.appendChild(moduleEl);

    // Host plugins don't send a moduleDidLoad message. We'll fake it here.
    if (isHost) {
      window.setTimeout(function () {
        var evt = document.createEvent('Event');
        evt.initEvent('load', true, true);  // bubbles, cancelable
        moduleEl.dispatchEvent(evt);
      }, 100);  // 100 ms
    }
  }

  /**
   * Add the default "load" and "message" event listeners to the element with
   * id "listener".
   *
   * The "load" event is sent when the module is successfully loaded. The
   * "message" event is sent when the naclModule posts a message using
   * PPB_Messaging.PostMessage() (in C) or pp::Instance().PostMessage() (in
   * C++).
   */
  function attachDefaultListeners() {
    var listenerDiv = document.getElementById('listener');
    listenerDiv.addEventListener('load', moduleDidLoad, true);
    listenerDiv.addEventListener('message', handleMessage, true);

    if (typeof window.attachListeners !== 'undefined') {
      window.attachListeners();
    }
  }

  /**
   * Called when the NaCl module is loaded.
   *
   * This event listener is registered in createNaClModule above.
   */
  function moduleDidLoad() {
    common.naclModule = document.getElementById('nacl_module');
    updateStatus('SUCCESS');

    if (typeof window.moduleDidLoad !== 'undefined') {
      window.moduleDidLoad();
    }
  }

  /**
   * Hide the NaCl module's embed element.
   *
   * We don't want to hide by default; if we do, it is harder to determine that
   * a plugin failed to load. Instead, call this function inside the example's
   * "moduleDidLoad" function.
   *
   */
  function hideModule() {
    // Setting common.naclModule.style.display = "None" doesn't work; the
    // module will no longer be able to receive postMessages.
    common.naclModule.style.height = "0";
  }

  /**
   * Return true when |s| starts with the string |prefix|.
   *
   * @param {string} s The string to search.
   * @param {string} prefix The prefix to search for in |s|.
   */
  function startsWith(s, prefix) {
    // indexOf would search the entire string, lastIndexOf(p, 0) only checks at
    // the first index. See: http://stackoverflow.com/a/4579228
    return s.lastIndexOf(prefix, 0) === 0;
  }

  /**
   * Add a message to an element with id "log", separated by a <br> element.
   *
   * This function is used by the default "log:" message handler.
   *
   * @param {string} message The message to log.
   */
  function logMessage(message) {
    var logEl = document.getElementById('log');
    logEl.innerHTML += message + '<br>';
    console.log(message)
  }

  /**
   */
  var defaultMessageTypes = {
    'alert': alert,
    'log': logMessage
  };

  /**
   * Called when the NaCl module sends a message to JavaScript (via
   * PPB_Messaging.PostMessage())
   *
   * This event listener is registered in createNaClModule above.
   *
   * @param {Event} message_event A message event. message_event.data contains
   *     the data sent from the NaCl module.
   */
  function handleMessage(message_event) {
    if (typeof message_event.data === 'string') {
      for (var type in defaultMessageTypes) {
        if (defaultMessageTypes.hasOwnProperty(type)) {
          if (startsWith(message_event.data, type + ':')) {
            func = defaultMessageTypes[type];
            func(message_event.data.slice(type.length + 1));
          }
        }
      }
    }

    if (typeof window.handleMessage !== 'undefined') {
      window.handleMessage(message_event);
    }
  }

  /**
   * Called when the DOM content has loaded; i.e. the page's document is fully
   * parsed. At this point, we can safely query any elements in the document via
   * document.querySelector, document.getElementById, etc.
   *
   * @param {string} name The name of the example.
   * @param {string} tool The name of the toolchain, e.g. "glibc", "newlib" etc.
   * @param {string} path Directory name where .nmf file can be found.
   * @param {number} width The width to create the plugin.
   * @param {number} height The height to create the plugin.
   */
  function domContentLoaded(name, tool, path, width, height) {
    // If the page loads before the Native Client module loads, then set the
    // status message indicating that the module is still loading.  Otherwise,
    // do not change the status message.
    updateStatus('Page loaded.');
    if (common.naclModule == null) {
      updateStatus('Creating embed: ' + tool)

      // We use a non-zero sized embed to give Chrome space to place the bad
      // plug-in graphic, if there is a problem.
      width = typeof width !== 'undefined' ? width : 200;
      height = typeof height !== 'undefined' ? height : 200;
      attachDefaultListeners();
      createNaClModule(name, tool, path, width, height);
    } else {
      // It's possible that the Native Client module onload event fired
      // before the page's onload event.  In this case, the status message
      // will reflect 'SUCCESS', but won't be displayed.  This call will
      // display the current message.
      updateStatus('Waiting.');
    }
  }

  /** Saved text to display in the element with id 'statusField'. */
  var statusText = 'NO-STATUSES';

  /**
   * Set the global status message. If the element with id 'statusField'
   * exists, then set its HTML to the status message as well.
   *
   * @param {string} opt_message The message to set. If null or undefined, then
   *     set element 'statusField' to the message from the last call to
   *     updateStatus.
   */
  function updateStatus(opt_message) {
    if (opt_message) {
      statusText = opt_message;
    }
    var statusField = document.getElementById('statusField');
    if (statusField) {
      statusField.innerHTML = statusText;
    }
  }

  // The symbols to export.
  return {
    /** A reference to the NaCl module, once it is loaded. */
    naclModule: null,

    attachDefaultListeners: attachDefaultListeners,
    domContentLoaded: domContentLoaded,
    createNaClModule: createNaClModule,
    hideModule: hideModule,
    updateStatus: updateStatus
  };

}());

// Listen for the DOM content to be loaded. This event is fired when parsing of
// the page's document has finished.
document.addEventListener('DOMContentLoaded', function() {
  var body = document.querySelector('body');

  // The data-* attributes on the body can be referenced via body.dataset.
  if (body.dataset) {
    var loadFunction;
    if (!body.dataset.customLoad) {
      loadFunction = common.domContentLoaded;
    } else if (typeof window.domContentLoaded !== 'undefined') {
      loadFunction = window.domContentLoaded;
    }

    if (loadFunction) {
      loadFunction(body.dataset.name, body.dataset.tc, body.dataset.path,
          body.dataset.width, body.dataset.height);
    }
  }
});
