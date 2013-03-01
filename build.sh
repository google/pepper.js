#!/bin/sh

# Build config
EMCC=../emscripten/emcc
NACL_SDK_ROOT=../../nacl_sdk/pepper_25
set -x verbose
set -e

# Make sure the main output directory exists.
mkdir -p out

IN_DIR=examples/hello_world
OUT_DIR=out/hello_world

# Make sure the output directory exists.
mkdir -p $OUT_DIR

# Stage data requried to run in the browser.
cp examples/common.js $IN_DIR/*.html $IN_DIR/*.js $OUT_DIR

# Compile the hello world example.
$EMCC -std=c++11 -I$NACL_SDK_ROOT/include ppapi_stub.cc $IN_DIR/hello_world.cc --js-library library_ppapi.js --pre-js ppapi.js --pre-js module.js -o $OUT_DIR/hello_world.js

# Files that are normally compiled into libppapi_cpp.
D=$NACL_SDK_ROOT/src/ppapi_cpp
PPAPI_CPP="$D/ppp_entrypoints.cc $D/module.cc $D/instance.cc $D/instance_handle.cc $D/var.cc $D/url_request_info.cc $D/url_response_info.cc $D/url_loader.cc $D/resource.cc $D/lock.cc"

IN_DIR=examples/geturl
OUT_DIR=out/geturl

# Make sure the output directory exists.
mkdir -p $OUT_DIR

# Stage data requried to run in the browser.
cp examples/common.js $IN_DIR/*.html $IN_DIR/*.js $OUT_DIR

# Compile the geturl example.
$EMCC -std=c++11 -I$NACL_SDK_ROOT/include ppapi_stub.cc $PPAPI_CPP $IN_DIR/geturl.cc $IN_DIR/geturl_handler.cc --js-library library_ppapi.js --pre-js module.js --pre-js ppapi.js -o $OUT_DIR/geturl.js -s EXPORTED_FUNCTIONS="['_main', '_RunCompletionCallback']"
