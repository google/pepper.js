#!/bin/sh
EMCC=../emscripten/emscripten/emcc
NACL_SDK_ROOT=../nacl_sdk/pepper_25
set -x verbose

mkdir -p out
$EMCC -std=c++11 -I$NACL_SDK_ROOT/include ppapi_stub.cc examples/hello_world/hello_world.cc --js-library library_ppapi.js -o out/hello_world.js
cp examples/hello_world/hello_world.html out/