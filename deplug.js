var postMessage = function(message) {
    // HACK assumes string.
    ptr = allocate(intArrayFromString(message), 'i8', ALLOC_NORMAL);
    _DoPostMessage(1, ptr);
    _free(ptr);
}

var ResourceManager = function() {
    this.lut = {};
    this.uid = 1;
}

ResourceManager.prototype.register = function(type, res) {
    while (this.uid in this.lut) {
        this.uid = (this.uid + 1) & 0xffffffff;
    }
    res.type = type;
    res.uid = this.uid;
    res.refcount = 1;
    this.lut[res.uid] = res;
    //console.log("create", res.uid);
    return this.uid;
}

ResourceManager.prototype.resolve = function(res) {
    if (typeof res === 'number')
	return this.lut[res]
    else
	return res;
}

ResourceManager.prototype.is = function(res, type) {
    var res = this.resolve(res);
    return res && res.type == type;
}

ResourceManager.prototype.addRef = function(uid) {
    var res = this.resolve(uid);
    //console.log("inc", uid);
    if (res === undefined) {
	throw "Resource does not exist.";
    }
    res.refcount += 1;
}

ResourceManager.prototype.release = function(uid) {
    var res = this.resolve(uid);
    //console.log("dec", uid);
    if (res === undefined) {
	throw "Resource does not exist.";
    }
    res.refcount -= 1;
    if (res.refcount <= 0) {
	if (res.destroy) {
	    res.destroy();
	}
	delete this.lut[res.uid];
    }
}

// HACK
var resources = new ResourceManager();

// Entry point
CreateInstance = function(width, height) {
    var shadow_instance = document.createElement('canvas');
    shadow_instance.setAttribute('name', 'nacl_module');
    shadow_instance.setAttribute('id', 'nacl_module');
    shadow_instance.setAttribute('width', width);
    shadow_instance.setAttribute('height', height);
    shadow_instance.postMessage = postMessage;
    // HACK global
    fakeEmbed = shadow_instance;

    // HACK more globals
    ctx = fakeEmbed.getContext('2d');
    imageData = ctx.createImageData(200, 200);
    mapped_buffer = new Uint32Array(imageData.data.buffer);

    var instance = 0;

    shadow_instance.addEventListener('DOMNodeInserted', function(evt) {
	instance = _NativeCreateInstance();

	// Create and send a bogus view resource.
	var view = resources.register("view", {
	    rect: {x: 0, y: 0, width: width, height: height},
	    fullscreen: true,
	    visible: true,
	    page_visible: true,
	    clip_rect: {x: 0, y: 0, width: width, height: height}
	});
	_DoChangeView(instance, view);
	resources.release(view);

	// Fake the load event.
        var evt = document.createEvent('Event');
        evt.initEvent('load', true, true);  // bubbles, cancelable
        shadow_instance.dispatchEvent(evt);
    }, true);

    // TODO handle removal events.

    return shadow_instance;
}