// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var rawEarthPixels;

function moduleDidLoad() {
  // Load earth image from 1024x512 jpg, decompress into canvas.
  var img = new Image();
  img.src = 'earth1k.jpg';
  var graph = document.createElement('canvas');
  graph.width = img.width;
  graph.height = img.height;
  var context = graph.getContext('2d');
  context.drawImage(img, 0, 0);
  var imageData = context.getImageData(0, 0, 1024, 512);
  rawEarthPixels = imageData.data.buffer;
}

function postThreadFunc(numThreads) {
  return function () {
    common.naclModule.postMessage('threads: ' + numThreads);
  }
}

// Add event listeners after the NaCl module has loaded.  These listeners will
// forward messages to the NaCl module via postMessage()
function attachListeners() {
  document.getElementById('benchmark').addEventListener('click',
    function() {
      common.naclModule.postMessage('run benchmark');
      common.updateStatus('BENCHMARKING... (please wait)');
    });
  // Disable controls for threading if threading is not supported.
  var disabled = document.location.search.search("emscripten") != -1;
  var threads = [0, 1, 2, 4, 6, 8, 12, 16, 24, 32];
  for (var i = 0; i < threads.length; i++) {
    var e = document.getElementById('radio' + i);
    // Disable controls for threading if threading is not supported.
    e.disabled = disabled;
    e.addEventListener('click', postThreadFunc(threads[i]));
  }
  document.getElementById('zoomRange').addEventListener('change',
    function() {
      var value = document.getElementById('zoomRange').value;
      common.naclModule.postMessage('zoom: ' + value);
    });
  document.getElementById('lightRange').addEventListener('change',
    function() {
      var value = document.getElementById('lightRange').value;
      common.naclModule.postMessage('light: ' + value);
    });
}

// Handle a message coming from the NaCl module.
function handleMessage(message_event) {
  if (message_event.data.indexOf('result:') != -1) {
    // benchmark result
    var valueStr = message_event.data.substr(message_event.data.indexOf(' '));
    var x = parseFloat(valueStr);
    console.log('Benchmark result: ' + x);
    var s = (Math.round(x * 1000) / 1000).toFixed(3);
    document.getElementById('result').textContent =
      'Result: ' + s + ' seconds';
    common.updateStatus('SUCCESS');
  } else if (message_event.data.indexOf('zoom:') != -1) {
    // zoom slider
    var valueStr = message_event.data.substr(message_event.data.indexOf(' '));
    var x = parseFloat(valueStr);
    document.getElementById('zoomRange').value = x;
  } else if (message_event.data.indexOf('light:') != -1) {
    // zoom slider
    var valueStr = message_event.data.substr(message_event.data.indexOf(' '));
    var x = parseFloat(valueStr);
    document.getElementById('lightRange').value = x;
  } else if (message_event.data.indexOf('request_image:') != -1) {
    var name = message_event.data.substr(message_event.data.indexOf(' ') + 1);
    // Load image from jpg, decompress into canvas.
    var img = new Image();
    img.onload = function() {
      var graph = document.createElement('canvas');
      graph.width = img.width;
      graph.height = img.height;
      var context = graph.getContext('2d');
      context.drawImage(img, 0, 0);
      var imageData = context.getImageData(0, 0, img.width, img.height);
      // Send NaCl module the raw image data obtained from canvas.
      common.naclModule.postMessage('tex_width: ' + img.width);
      common.naclModule.postMessage('tex_height: ' + img.height);
      common.naclModule.postMessage(common.getImageDataBuffer(imageData));
    }
    img.src = name;
  }
}

