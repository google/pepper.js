var ppapi_exports = {
  GetBrowserInterface: function(interface_name) {
    var name = Pointer_stringify(interface_name);
    if (!(name in interfaces)) {
      console.error('Requested unknown interface: ' + name);
      return 0;
    }
    var inf = interfaces[name]|0;
    if (inf === 0) {
      // The interface exists, but it is not available for this particular browser.
      console.error('Requested unavailable interface: ' + name);
    }
    return inf;
  },
};

mergeInto(LibraryManager.library, ppapi_exports);
