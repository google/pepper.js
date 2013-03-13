// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Called by the common.js module.
function moduleDidLoad() {
  // The module is not hidden by default so we can easily see if the plugin
  // failed to load.
  common.hideModule();
}

// Called by the common.js module.
function attachListeners() {
  document.getElementById('button').addEventListener('click', loadUrl);
}

function loadUrl() {
  common.naclModule.postMessage('getUrl:geturl_success.html');
}

// Called by the common.js module.
function handleMessage(message_event) {
  var logElt = document.getElementById('generalOutput');
  // Find the first line break.  This separates the URL data from the
  // result text.  Note that the result text can contain any number of
  // '\n' characters, so split() won't work here.
  var url = message_event.data;
  var result = '';
  var eol_pos = message_event.data.indexOf("\n");
  if (eol_pos != -1) {
    url = message_event.data.substring(0, eol_pos);
    if (eol_pos < message_event.data.length - 1) {
      result = message_event.data.substring(eol_pos + 1);
    }
  }
  logElt.textContent += 'FULLY QUALIFIED URL: ' + url + '\n';
  logElt.textContent += 'RESULT:\n' + result + '\n';
}
