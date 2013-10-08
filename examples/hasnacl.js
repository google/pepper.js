var nacl = nacl || {};

(function(nacl) {
  nacl.chromeMajorVersion = function() {
    var re = /Chrome\/(\d+)\.(\d+)\.(\d+)\.(\d+)/;
    var result = re.exec(navigator.userAgent);
    if (!result)
      return null;
    return +result[1];
  };

  nacl.naclMimeType = 'application/x-nacl';

  nacl.hasNaCl = function() {
    return navigator.mimeTypes[nacl.naclMimeType] !== undefined;
  };

  nacl.pnaclMimeType = 'application/x-pnacl';

  nacl.hasPNaCl = function() {
    return navigator.mimeTypes[nacl.pnaclMimeType] !== undefined;
  };

  nacl.hasEmscripten = function() {
    return window.ArrayBuffer !== undefined;
  };

  nacl.hasWebGL = function() {
    var c = document.createElement("canvas");
    var ctx = c.getContext('webgl') || c.getContext("experimental-webgl");
    return !!ctx;
  };

  nacl.hasFullscreen = function() {
    var b = document.body;
    return !!(b.requestFullscreen || b.mozRequestFullScreen || b.webkitRequestFullscreen);
  };

  nacl.hasPointerLock = function() {
    var b = document.body;
    return (b.webkitRequestPointerLock || b.mozRequestPointerLock);
  };

  nacl.hasWebAudio = function() {
    return !!(window["AudioContext"] || window["webkitAudioContext"]);
  };
})(nacl);