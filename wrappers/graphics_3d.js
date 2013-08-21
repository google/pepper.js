(function() {

  var getContext = function(c) {
    return c.getContext('webgl') || c.getContext("experimental-webgl");
  };

  var Graphics3D_GetAttribMaxValue = function(instance, attribute, value) {
    throw "Graphics3D_GetAttribMaxValue not implemented";
  };

  var Graphics3D_Create = function(instance, share_context, attrib_list) {
    var i = resources.resolve(instance, INSTANCE_RESOURCE);
    if (i === undefined) {
      return 0;
    }
    if (share_context !== 0) {
      throw "Graphics3D shared contexts not supported.";
    }

    // TODO(ncbray): support attribs.
    // TODO(ncbray): is the canvas opaque?
    var canvas = i.createCanvas(i.element.getAttribute("width"), i.element.getAttribute("height"), false);
    canvas.style.width = "100%";
    canvas.style.height = "100%";

    return resources.register(GRAPHICS_3D_RESOURCE, {
      canvas: canvas,
      bound: false,
      ctx: getContext(canvas),
      notifyBound: function(instance) {
        this.bound = true;
        instance.element.appendChild(this.canvas);
      },
      notifyUnbound: function(instance) {
        instance.element.removeChild(this.canvas);
        this.bound = false;
      }
    });
  };

  var Graphics3D_IsGraphics3D = function(resource) {
    return resources.is(resource, GRAPHICS_3D_RESOURCE);
  };

  var Graphics3D_GetAttribs = function(context, attrib_list) {
    throw "Graphics3D_GetAttribs not implemented";
  };

  var Graphics3D_SetAttribs = function(context, attrib_list) {
    throw "Graphics3D_SetAttribs not implemented";
  };

  var Graphics3D_GetError = function(context) {
    throw "Graphics3D_GetError not implemented";
  };

  var Graphics3D_ResizeBuffers = function(context, width, height) {
    throw "Graphics3D_ResizeBuffers not implemented";
  };

  var Graphics3D_SwapBuffers = function(context, callback) {
    // TODO double buffering.
    var c = ppapi_glue.convertCompletionCallback(callback);
    Module.requestAnimationFrame(function() {
      c(0);
    });
  };

  registerInterface("PPB_Graphics3D;1.0", [
    Graphics3D_GetAttribMaxValue,
    Graphics3D_Create,
    Graphics3D_IsGraphics3D,
    Graphics3D_GetAttribs,
    Graphics3D_SetAttribs,
    Graphics3D_GetError,
    Graphics3D_ResizeBuffers,
    Graphics3D_SwapBuffers,
  ], function() {
      return !!getContext(document.createElement("canvas"));
  });
})();
