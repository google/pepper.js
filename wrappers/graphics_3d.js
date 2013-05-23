(function() {

    var Graphics3D_GetAttribMaxValue = function(instance, attribute, value) {
        throw "Graphics3D_GetAttribMaxValue not implemented";
    };

    var Graphics3D_Create = function(instance, share_context, attrib_list) {
	   var canvas = resources.resolve(instance).canvas;

        var resource = resources.register("graphics_3d", {
	    canvas: canvas,
	    ctx: canvas.getContext('webgl') || canvas.getContext("experimental-webgl"),
  	    destroy: function() {
		throw "Graphics3D destroy not implemented.";
            }
	});
	return resource;
    };

    var Graphics3D_IsGraphics3D = function(resource) {
	return resources.is(resource, "graphics_3d");
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
    ]);
})();
