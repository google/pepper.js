# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#
# GNU Make based build file.  For details on GNU Make see:
#   http://www.gnu.org/software/make/manual/make.html
#
#

# Default configuration
#
# By default we will build a Debug configuration using the GCC newlib toolcahin
# to override this, specify TOOLCHAIN=newlib|glibc or CONFIG=Debug|Release on
# the make command-line or in this file prior to including common.mk.  The
# toolchain we use by default will be the first valid one listed
VALID_TOOLCHAINS:=emscripten newlib pnacl

PPAPIJS_SRC_ROOT:=../..
EXTRA_INC_PATHS=
include $(PPAPIJS_SRC_ROOT)/tools/common.mk

#
# Target Name
#
# The base name of the final library, also the name of the NMF file containing
# the mapping between architecture and actual NEXE.
#
TARGET=ppapi_cpp

#
# List of sources to compile
#
SOURCES= \
  array_output.cc \
  audio.cc \
  audio_config.cc \
  core.cc \
  file_io.cc \
  file_ref.cc \
  file_system.cc \
  fullscreen.cc \
  graphics_2d.cc \
  graphics_3d.cc \
  graphics_3d_client.cc \
  image_data.cc \
  input_event.cc \
  instance.cc \
  instance_handle.cc \
  lock.cc \
  message_loop.cc \
  module.cc \
  mouse_cursor.cc \
  mouse_lock.cc \
  paint_aggregator.cc \
  paint_manager.cc \
  ppp_entrypoints.cc \
  rect.cc \
  resource.cc \
  simple_thread.cc \
  url_loader.cc \
  url_request_info.cc \
  url_response_info.cc \
  var.cc \
  var_array_buffer.cc \
  view.cc \
  websocket.cc \
  websocket_api.cc \

SOURCES:=$(foreach src,$(SOURCES),$(NACL_SDK_ROOT)/src/ppapi_cpp/$(src))

#
# Use the compile macro for each source.
#
$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),)))

#
# Use the lib macro for this target on the list of sources.
#
$(eval $(call LIB_RULE,ppapi_cpp,$(SOURCES)))
