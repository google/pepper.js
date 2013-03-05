var postMessage = function(message) {
    // HACK assumes string.
    ptr = allocate(intArrayFromString(message), 'i8', ALLOC_NORMAL);
    _DoPostMessage(1, ptr);
    _free(ptr);
}

// Entry point
CreateInstance = function(width, height) {
    var shadow_instance = document.createElement('embed');
    shadow_instance.setAttribute('name', 'nacl_module');
    shadow_instance.setAttribute('id', 'nacl_module');
    shadow_instance.setAttribute('width', width);
    shadow_instance.setAttribute('height', height);
    shadow_instance.postMessage = postMessage;
    // HACK global
    fakeEmbed = shadow_instance;

    var instance = 0;

    shadow_instance.addEventListener('DOMNodeInserted', function(evt) {
	instance = _NativeCreateInstance();
	//_DoChangeView(instance, 101);

	// Fake the load event.
        var evt = document.createEvent('Event');
        evt.initEvent('load', true, true);  // bubbles, cancelable
        shadow_instance.dispatchEvent(evt);
    }, true);

    // TODO handle removal events.

    return shadow_instance;
}