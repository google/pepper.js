(function() {

  var Graphics2D_Create = function(instance, size_ptr, is_always_opaque) {
    var size = ppapi_glue.getSize(size_ptr);
    var canvas = resources.resolve(instance).canvas;
    canvas.width = size.width;
    canvas.height = size.height;

    var resource = resources.register("graphics_2d", {
      size: size,
      canvas: canvas,
      ctx: canvas.getContext('2d'),
      always_opaque: true,
      destroy: function() {
        //TODO(grosse): recreate canvas when necessary
	// throw "Canvas destroy not implemented.";
      }
    });
    return resource;
  };

  var Graphics2D_IsGraphics2D = function(resource) {
    return resources.is(resource, "graphics_2d");
  };

  var Graphics2D_Describe = function(resource, size_ptr, is_always_opqaue_ptr) {
    throw "Graphics2D_Describe not implemented";
  };

  var Graphics2D_PaintImageData = function(resource, image_data, top_left_ptr, src_rect_ptr) {
    if (src_rect_ptr !== 0){
      throw "Graphics2D_PaintImageData doesn't support nonnull src_rect argument yet";
    }

    var g2d = resources.resolve(resource);
    var res = resources.resolve(image_data);
    res.image_data.data.set(res.view);

    var top_left = ppapi_glue.getPoint(top_left_ptr);
    g2d.ctx.putImageData(res.image_data, top_left.x, top_left.y);
  };

  var Graphics2D_Scroll = function(resource, clip_rect_ptr, amount_ptr) {
    throw "Graphics2D_Scroll not implemented";
  };

  var Graphics2D_ReplaceContents = function(resource, image_data) {
    var g2d = resources.resolve(resource);
    var res = resources.resolve(image_data);
    res.image_data.data.set(res.view);

    g2d.ctx.putImageData(res.image_data, 0, 0);
  };

  var Graphics2D_Flush = function(resource, callback) {
    var c = ppapi_glue.convertCompletionCallback(callback);
    Module.requestAnimationFrame(function() {
      c(ppapi.PP_Error.PP_OK);
    });
    return ppapi.PP_Error.PP_OK;
  };

  registerInterface("PPB_Graphics2D;1.0", [
    Graphics2D_Create,
    Graphics2D_IsGraphics2D,
    Graphics2D_Describe,
    Graphics2D_PaintImageData,
    Graphics2D_Scroll,
    Graphics2D_ReplaceContents,
    Graphics2D_Flush
  ]);


  var ImageData_GetNativeImageDataFormat = function() {
    throw "ImageData_GetNativeImageDataFormat not implemented";
  };

  var ImageData_IsImageDataFormatSupported = function(format) {
    throw "ImageData_IsImageDataFormatSupported not implemented";
  };

  var ImageData_Create = function(instance, format, size_ptr, init_to_zero) {
    var size = ppapi_glue.getSize(size_ptr);
    var bytes = size.width * size.height * 4
    var memory = _malloc(bytes);
    // Note: "buffer" is an implementation detail of Emscripten and is likely not a stable interface.
    var view = new Uint8ClampedArray(buffer, memory, bytes);
    // Due to limitations of the canvas API, we need to create an intermediate "ImageData" buffer.
    // HACK for creating an image data without having a 2D context available.
    var c = document.createElement('canvas');
    var ctx = c.getContext('2d')
    var image_data = ctx.createImageData(size.width, size.height);

    var uid = resources.register("image_data", {
      format: format,
      size: size,
      memory: memory,
      view: view,
      image_data: image_data,
      destroy: function() {
        _free(memory);
      }
    });
    return uid;
  };

  var ImageData_IsImageData = function (image_data) {
    return resources.is("image_data", image_data);
  };

  var ImageData_Describe = function(image_data, desc_ptr) {
    //if (!resources.is("image_data", image_data)) return 0;

    var res = resources.resolve(image_data);
    setValue(desc_ptr + 0, res.format, 'i32');
    setValue(desc_ptr + 4, res.size.width, 'i32');
    setValue(desc_ptr + 8, res.size.height, 'i32');
    setValue(desc_ptr + 12, res.size.width*4, 'i32');
    return 1;
  };

  var ImageData_Map = function(image_data) {
    return resources.resolve(image_data).memory;
  };

  var ImageData_Unmap = function(image_data) {
    // Ignore
  };

  registerInterface("PPB_ImageData;1.0", [
    ImageData_GetNativeImageDataFormat,
    ImageData_IsImageDataFormatSupported,
    ImageData_Create,
    ImageData_IsImageData,
    ImageData_Describe,
    ImageData_Map,
    ImageData_Unmap
  ]);
})();