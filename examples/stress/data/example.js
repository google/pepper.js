// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var kMaxArraySize = 20;
var messageArray = new Array();

var Config = function() {
  this.dx = 3;
  this.dy = 2;
};

var config = new Config();

function moduleDidLoad() {
  var gui = new dat.GUI();
  var sync = function() {
    common.naclModule.postMessage(config);
  };
  gui.add(config, 'dx', -5, 5).step(1).onFinishChange(sync);
  gui.add(config, 'dy', -5, 5).step(1).onFinishChange(sync);
}

// Called by the common.js module.
function handleMessage(message) {
  // Show last |kMaxArraySize| events in html.
  messageArray.push(message.data);
  if (messageArray.length > kMaxArraySize) {
    messageArray.shift();
  }
  var newData = messageArray.join('<BR>');
  document.getElementById('outputString').innerHTML = newData;
  // Print event to console.
  console.log("Got post message: " + JSON.stringify(message.data));
  console.log(message.data);
}
