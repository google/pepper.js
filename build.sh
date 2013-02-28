#!/bin/sh

# Build config
EMCC=../emscripten/emcc
NACL_SDK_ROOT=../../nacl_sdk/pepper_25
set -x verbose
set -e

# Make sure the output directory exists.
mkdir -p out

# Compile the hello world example.
$EMCC -std=c++11 -I$NACL_SDK_ROOT/include ppapi_stub.cc examples/hello_world/hello_world.cc --js-library library_ppapi.js --pre-js ppapi.js --pre-js module.js -o out/hello_world.js

# Stage data requried to run in the browser.
cp examples/hello_world/*.html out/
cp examples/hello_world/*.js out/

# Files that are normally compiled into libppapi_cpp.
D=$NACL_SDK_ROOT/src/ppapi_cpp
PPAPI_CPP="$D/ppp_entrypoints.cc $D/module.cc $D/instance.cc $D/instance_handle.cc $D/var.cc $D/url_request_info.cc $D/url_response_info.cc $D/url_loader.cc $D/resource.cc $D/lock.cc"

# Compile the geturl example.
$EMCC -std=c++11 -I$NACL_SDK_ROOT/include ppapi_stub.cc $PPAPI_CPP examples/geturl/geturl.cc examples/geturl/geturl_handler.cc --js-library library_ppapi.js --pre-js module.js --pre-js ppapi.js -o out/geturl.js -s EXPORTED_FUNCTIONS="['_main', '_RunCompletionCallback']"

# Stage data requried to run in the browser.
cp examples/geturl/geturl.html out/
