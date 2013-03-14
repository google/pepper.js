(function() {
    var View_IsView = function(resource) {
	return resources.is(resource, "view");
    };

    var View_GetRect = function(resource, rectptr) {
	ppapi_glue.setRect(resources.resolve(resource).rect, rectptr);
	return true;
    };

    var View_IsFullscreen = function(resource) {
	return resources.resolve(resource).fullscreen;
    };

    var View_IsVisible = function(resource) {
	return resources.resolve(resource).visible;
    };

    var View_IsPageVisible = function(resource) {
	return resources.resolve(resource).page_visible;
    };

    var View_GetClipRect = function(resource, rectptr) {
	ppapi_glue.setRect(resources.resolve(resource).clip_rect, rectptr);
	return true;
    };

    registerInterface("PPB_View;1.0", [
	View_IsView,
	View_GetRect,
	View_IsFullscreen,
	View_IsVisible,
	View_IsPageVisible,
	View_GetClipRect,
    ]);
})();
