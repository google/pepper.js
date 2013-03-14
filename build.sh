#!/bin/sh

if [ -z "$1" ]; then
MODE=debug
else
MODE=$1
fi

if [ $MODE = "debug" ]; then
MODE_FLAGS="-O0 --closure 0 --minify 0"
elif [ $MODE = "release" ]; then
MODE_FLAGS="-O2 --closure 1 --minify 1"
else
echo "unknown mode: $MODE"
exit 1
fi
BUILD_ROOT="out/$MODE"


# Build config
EMCC=../emscripten/emcc
NACL_SDK_ROOT=../../nacl_sdk/pepper_25
set -x verbose
set -e

# Make sure the main output directory exists.
mkdir -p $BUILD_ROOT

FLAGS="-std=c++11 $MODE_FLAGS --js-library library_ppapi.js --pre-js deplug.js --pre-js ppapi.js --post-js wrappers/view.js -I$NACL_SDK_ROOT/include"

# Libraries
mkdir -p $BUILD_ROOT/lib
D=$NACL_SDK_ROOT/src/ppapi_cpp
PPAPI_CPP_FILES="$D/ppp_entrypoints.cc $D/module.cc $D/instance.cc $D/instance_handle.cc $D/var.cc $D/url_request_info.cc $D/url_response_info.cc $D/url_loader.cc $D/resource.cc $D/lock.cc $D/input_event.cc $D/view.cc $D/graphics_2d.cc $D/image_data.cc $D/core.cc"
PPAPI_CPP=$BUILD_ROOT/lib/ppapi_cpp.o
$EMCC $FLAGS $PPAPI_CPP_FILES -o $PPAPI_CPP

# Examples
example() {
  NAME=$1
  IN_DIR=examples/$NAME
  OUT_DIR=$BUILD_ROOT/$NAME

  # Make sure the output directory exists.
  mkdir -p $OUT_DIR

  # Stage data requried to run in the browser.
  cp examples/common.js $IN_DIR/data/* $OUT_DIR
}

example "hello_world"
SOURCES="$IN_DIR/hello_world.cc"
$EMCC $FLAGS deplug.cc $SOURCES -o $OUT_DIR/$NAME.js -s EXPORTED_FUNCTIONS="['CreateInstance']"

example "geturl"
SOURCES="$IN_DIR/geturl.cc $IN_DIR/geturl_handler.cc"
$EMCC $FLAGS $PPAPI_CPP deplug.cc $SOURCES -o $OUT_DIR/$NAME.js -s EXPORTED_FUNCTIONS="['CreateInstance']"

example "pi_generator"
SOURCES="$IN_DIR/pi_generator.cc $IN_DIR/pi_generator_module.cc"
$EMCC $FLAGS -I . deplug.cc $PPAPI_CPP $SOURCES -o $OUT_DIR/$NAME.js -s "EXPORTED_FUNCTIONS=['CreateInstance']"

# Native version.
NEXE=$OUT_DIR/${NAME}_x86_64.nexe
GCC=$NACL_SDK_ROOT/toolchain/linux_x86_newlib/bin/x86_64-nacl-g++
$GCC -m64 -std=gnu++98 -O2 -I$NACL_SDK_ROOT/include $SOURCES -lppapi_cpp -lppapi -lpthread -lplatform -o $NEXE
strip $NEXE
