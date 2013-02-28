#!/bin/sh
EMCC=../emscripten/emscripten/emcc
NACL_SDK_ROOT=../nacl_sdk/pepper_25
set -x verbose
set -e

mkdir -p out

$EMCC -std=c++11 -I$NACL_SDK_ROOT/include ppapi_stub.cc examples/hello_world/hello_world.cc --js-library library_ppapi.js --pre-js ppapi.js -o out/hello_world.js

cp examples/hello_world/hello_world.html out/

D=$NACL_SDK_ROOT/src/ppapi_cpp
PPAPI_CPP="$D/ppp_entrypoints.cc $D/module.cc $D/instance.cc $D/instance_handle.cc $D/var.cc $D/url_request_info.cc $D/url_response_info.cc $D/url_loader.cc $D/resource.cc $D/lock.cc"

$EMCC -std=c++11 -I$NACL_SDK_ROOT/include ppapi_stub.cc $PPAPI_CPP examples/geturl/geturl.cc examples/geturl/geturl_handler.cc --js-library library_ppapi.js --pre-js ppapi.js -o out/geturl.js -s EXPORTED_FUNCTIONS="['_main', '_RunCompletionCallback']"

cp examples/geturl/geturl.html out/
