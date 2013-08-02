(function() {

  var Graphics2D_Create = function(instance, size_ptr, is_always_opaque) {
    var size = ppapi_glue.getSize(size_ptr);
    var i = resources.resolve(instance, INSTANCE_RESOURCE);
    if (i === undefined) {
      return 0;
    }
    var canvas = i.canvas;
    canvas.width = size.width;
    canvas.height = size.height;

    return resources.register(GRAPHICS_2D_RESOURCE, {
      size: size,
      canvas: canvas,
      ctx: canvas.getContext('2d'),
      always_opaque: true,
      destroy: function() {
        //TODO(grosse): recreate canvas when necessary
	// throw "Canvas destroy not implemented.";
      }
    });
  };

  var Graphics2D_IsGraphics2D = function(resource) {
    return resources.is(resource, GRAPHICS_2D_RESOURCE);
  };

  var Graphics2D_Describe = function(resource, size_ptr, is_always_opqaue_ptr) {
    throw "Graphics2D_Describe not implemented";
  };

  var Graphics2D_PaintImageData = function(resource, image_data, top_left_ptr, src_rect_ptr) {
    if (src_rect_ptr !== 0){
      throw "Graphics2D_PaintImageData doesn't support nonnull src_rect argument yet";
    }
    var g2d = resources.resolve(resource, GRAPHICS_2D_RESOURCE);
    // Eat any errors that occur, same as the implementation in Chrome.
    if (g2d === undefined) {
      return;
    }
    var res = resources.resolve(image_data, IMAGE_DATA_RESOURCE);
    if (res === undefined) {
      return;
    }
    syncImageData(res);
    var top_left = ppapi_glue.getPoint(top_left_ptr);
    g2d.ctx.putImageData(res.image_data, top_left.x, top_left.y);
  };

  var Graphics2D_Scroll = function(resource, clip_rect_ptr, amount_ptr) {
    throw "Graphics2D_Scroll not implemented";
  };

  var Graphics2D_ReplaceContents = function(resource, image_data) {
    var g2d = resources.resolve(resource, GRAPHICS_2D_RESOURCE);
    // Eat any errors that occur, same as the implementation in Chrome.
    if (g2d === undefined) {
      return;
    }
    var res = resources.resolve(image_data, IMAGE_DATA_RESOURCE);
    if (res === undefined) {
      return;
    }
    syncImageData(res);
    g2d.ctx.putImageData(res.image_data, 0, 0);
  };

  var Graphics2D_Flush = function(resource, callback) {
    var c = ppapi_glue.convertCompletionCallback(callback);
    Module.requestAnimationFrame(function() {
      c(ppapi.PP_OK);
    });
    return ppapi.PP_OK;
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

  // Copy the data from Emscripten's memory space into the ImageData object.
  var syncImageData = function(res) {
    if (res.view !== null) {
      res.image_data.data.set(res.view);
    } else {
      var image_data = res.image_data;
      var base = res.memory;
      var bytes = res.size.width * res.size.height * 4;

      if (res.format === 0) {
	// BGRA
	for (var i = 0; i < bytes; i += 4) {
	  image_data.data[i]     = HEAPU8[base + i + 2];
	  image_data.data[i + 1] = HEAPU8[base + i + 1];
	  image_data.data[i + 2] = HEAPU8[base + i];
	  image_data.data[i + 3] = HEAPU8[base + i + 3];
	}
      } else {
	// RGBA
	for (var i = 0; i < bytes; i += 4) {
	  image_data.data[i]     = HEAPU8[base + i];
	  image_data.data[i + 1] = HEAPU8[base + i + 1];
	  image_data.data[i + 2] = HEAPU8[base + i + 2];
	  image_data.data[i + 3] = HEAPU8[base + i + 3];
	}
      }
    }
  }


  var ImageData_GetNativeImageDataFormat = function() {
    // PP_IMAGEDATAFORMAT_RGBA_PREMUL
    return 1;
  };

  // We only support RGBA.
  // To simplify porting we're pretending that we also support BGRA and really giving an RGBA buffer instead.
  var ImageData_IsImageDataFormatSupported = function(format) {
    return format == 0 || format == 1;
  };

  var ImageData_Create = function(instance, format, size_ptr, init_to_zero) {
    if (!ImageData_IsImageDataFormatSupported(format)) {
      return 0;
    }
    var size = ppapi_glue.getSize(size_ptr);
    if (size.width <= 0 || size.height <= 0) {
      return 0;
    }

    // HACK for creating an image data without having a 2D context available.
    var c = document.createElement('canvas');
    var ctx = c.getContext('2d');
    var image_data;
    try {
      // Due to limitations of the canvas API, we need to create an intermediate "ImageData" buffer.
      image_data = ctx.createImageData(size.width, size.height);
    } catch(err) {
      // Calls in the try block may return range errors if the sizes are too big.
      return 0;
    }

    var bytes = size.width * size.height * 4;
    var memory = _malloc(bytes);
    if (memory === 0) {
      return 0;
    }
    if (init_to_zero) {
      _memset(memory, 0, bytes);
    }

    var view = null;
    // Direct copies are only supported if:
    // 1) The image format is RGBA
    // 2) The canvas API implements image_data.data as a typed array.
    // 3) Uint8ClampedArray is defined (this is likely redundant with condition 2)
    // Note that Closure appears to minimize window.Uint8ClampedArray.
    var fast_path_supported = format === 1 && "set" in image_data.data && window["Uint8ClampedArray"] !== undefined;
    if (fast_path_supported) {
      try {
        // Note: "buffer" is an implementation detail of Emscripten and is likely not a stable interface.
        view = new Uint8ClampedArray(buffer, memory, bytes);
      } catch(err) {
        _free(memory);
        return 0;
      }
    }

    var uid = resources.register(IMAGE_DATA_RESOURCE, {
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
    return resources.is(image_data, IMAGE_DATA_RESOURCE);
  };

  var ImageData_Describe = function(image_data, desc_ptr) {
    var res = resources.resolve(image_data, IMAGE_DATA_RESOURCE);
    if (res !== undefined) {
      setValue(desc_ptr + 0, res.format, 'i32');
      setValue(desc_ptr + 4, res.size.width, 'i32');
      setValue(desc_ptr + 8, res.size.height, 'i32');
      setValue(desc_ptr + 12, res.size.width*4, 'i32');
      return 1;
    } else {
      _memset(desc_ptr, 0, 16);
      return 0;
    }
  };

  var ImageData_Map = function(image_data) {
    var res = resources.resolve(image_data, IMAGE_DATA_RESOURCE);
    if (res === undefined) {
      return 0;
    }
    return res.memory;
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