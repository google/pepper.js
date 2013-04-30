"use strict";

var postMessage = function(message) {
    // HACK assumes string.
    var ptr = allocate(intArrayFromString(message), 'i8', ALLOC_NORMAL);
    _DoPostMessage(this.instance, ptr);
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

var resources = new ResourceManager();
var interfaces = {};

var registerInterface = function(name, functions) {
    // allocate(...) is bugged for non-i8 allocations, so do it manually
    var ptr = Runtime.staticAlloc(functions.length * 4);
    for (var i in functions) {
	// TODO what is the sig?
	setValue(ptr + i * 4, Runtime.addFunction(functions[i], 1), 'i32');
    }
    interfaces[name] = ptr;
};

var Module = {};

var CreateInstance = function(width, height) {
    var shadow_instance = document.createElement('span');
    shadow_instance.setAttribute('name', 'nacl_module');
    shadow_instance.setAttribute('id', 'nacl_module');
    shadow_instance.style.width = width + "px";
    shadow_instance.style.height = height + "px";
    shadow_instance.style.padding = "0px";
    shadow_instance.postMessage = postMessage;

    shadow_instance.addEventListener('DOMNodeInserted', function(evt) {
	if (evt.target !== shadow_instance) return;

	var instance = resources.register("instance", {
	    element: shadow_instance
	});
	// Allows shadow_instance.postMessage to work.
	// This is only a UID so there is no circular reference.
	shadow_instance.instance = instance;
	_NativeCreateInstance(instance);

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

// Entry point
window["CreateInstance"] = CreateInstance;