// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Add event listeners after the NaCl module has loaded.  These listeners will
// forward messages to the NaCl module via postMessage()
function moduleDidLoad() {
  document.getElementById("benchmark").addEventListener("click",
    function() {
      common.naclModule.postMessage("run benchmark");
      alert("Please wait while running benchmark.")
    }, false);
  document.getElementById("draw_points").addEventListener("click",
    function() {
      var checked = document.getElementById("draw_points").checked;
      if (checked)
        common.naclModule.postMessage("with points");
      else
        common.naclModule.postMessage("without points");
    }, false);
  document.getElementById("draw_interiors").addEventListener("click",
    function() {
      var checked = document.getElementById("draw_interiors").checked;
      if (checked)
        common.naclModule.postMessage("with interiors");
      else
        common.naclModule.postMessage("without interiors");
    }, false);
  document.getElementById("one").addEventListener("click",
    function() {
      common.naclModule.postMessage("threads: 1");
    }, false);
  document.getElementById("two").addEventListener("click",
    function() {
      common.naclModule.postMessage("threads: 2");
    }, false);
  document.getElementById("four").addEventListener("click",
    function() {
      common.naclModule.postMessage("threads: 4");
    }, false);
  document.getElementById("six").addEventListener("click",
    function() {
      common.naclModule.postMessage("threads: 6");
    }, false);
  document.getElementById("eight").addEventListener("click",
    function() {
      common.naclModule.postMessage("threads: 8");
    }, false);
  document.getElementById("twelve").addEventListener("click",
    function() {
      common.naclModule.postMessage("threads: 12");
    }, false);
  document.getElementById("sixteen").addEventListener("click",
    function() {
      common.naclModule.postMessage("threads: 16");
    }, false);
  document.getElementById("twentyfour").addEventListener("click",
    function() {
      common.naclModule.postMessage("threads: 24");
    }, false);
  document.getElementById("thirtytwo").addEventListener("click",
    function() {
      common.naclModule.postMessage("threads: 32");
    }, false);

  var buttons = document.getElementsByName("thread_count");
  var disabled = document.location.search.search("emscripten") != -1;
  for (var i = 0; i < buttons.length; i++) {
    buttons[i].disabled = disabled;
  }

  document.getElementById("point_range").addEventListener("change",
    function() {
      var value = document.getElementById("point_range").value;
      common.naclModule.postMessage("points: " + value);
      document.getElementById("point_count").textContent = value + " points";
    }, false);
}

// Handle a message coming from the NaCl module.
// In the Voronoi example, the only message will be the benchmark result.
function handleMessage(message_event) {
  var x = Math.round(message_event.data * 1000) / 1000;
  document.getElementById("result").textContent =
    "Result: " + x.toFixed(3) + " seconds";
}

