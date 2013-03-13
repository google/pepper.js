// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var kMaxArraySize = 20;
var messageArray = new Array();

// Once we load, hide the plugin
function moduleDidLoad() {
  common.hideModule();
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
  console.log(message.data);
}
