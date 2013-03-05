// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Start up the paint timer when the NaCl module has loaded.
function moduleDidLoad() {
  setInterval(postPaintMessage, 5);
}

function postPaintMessage() {
  common.naclModule.postMessage('paint');
}

// Handle a message coming from the NaCl module.  The message payload is
// assumed to contain the current estimated value of Pi.  Update the Pi
// text display with this value.
function handleMessage(message_event) {
  document.getElementById('pi').value = message_event.data;
}
