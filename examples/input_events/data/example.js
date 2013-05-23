// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var kMaxArraySize = 20;
var messageArray = new Array();

// Called by the common.js module.
function moduleDidLoad() {
  common.naclModule.style.backgroundColor = 'gray';
}

// Called by the common.js module.
function attachListeners() {
  document.getElementById('killButton').addEventListener('click', cancelQueue);
}

// Called by the common.js module.
function handleMessage(message) {
  // Show last |kMaxArraySize| events in html.
  messageArray.push(message.data);
  if (messageArray.length > kMaxArraySize) {
    messageArray.shift();
  }
  var newData = messageArray.join('<BR>');
  document.getElementById('eventString').innerHTML = newData;
  // Print event to console.
  console.log(message.data);
}

function cancelQueue() {
  if (common.naclModule == null) {
    console.log('Module is not loaded.');
    return;
  }
  common.naclModule.postMessage('CANCEL');
}
