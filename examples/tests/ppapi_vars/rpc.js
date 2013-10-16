// Copyright (c) 2013 Google Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var RPC = function(embed) {
  this.embed = embed;
};

RPC.prototype.emptyArray = function() {
  this.embed.postMessage(0);
};

RPC.prototype.simpleArray = function() {
  this.embed.postMessage(1);
};

RPC.prototype.emptyDictionary = function() {
  this.embed.postMessage(2);
};

RPC.prototype.emptyArrayBuffer = function() {
  this.embed.postMessage(3);
};

RPC.prototype.simpleArrayBuffer = function() {
  this.embed.postMessage(4);
};
