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

PEPPERJS_SRC_ROOT:=../..
EXTRA_INC_PATHS=
include $(PEPPERJS_SRC_ROOT)/tools/common.mk

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
  alarms_dev.cc \
  array_output.cc \
  audio.cc \
  audio_config.cc \
  audio_input_dev.cc \
  buffer_dev.cc \
  core.cc \
  crypto_dev.cc \
  cursor_control_dev.cc \
  device_ref_dev.cc \
  directory_entry.cc \
  event_base.cc \
  events_dev.cc \
  file_chooser_dev.cc \
  file_io.cc \
  file_ref.cc \
  file_system.cc \
  find_dev.cc \
  font_dev.cc \
  fullscreen.cc \
  graphics_2d.cc \
  graphics_2d_dev.cc \
  graphics_3d.cc \
  graphics_3d_client.cc \
  host_resolver.cc \
  image_data.cc \
  ime_input_event_dev.cc \
  input_event.cc \
  instance.cc \
  instance_handle.cc \
  lock.cc \
  memory_dev.cc \
  message_loop.cc \
  module.cc \
  mouse_cursor.cc \
  mouse_lock.cc \
  net_address.cc \
  network_list.cc \
  network_monitor.cc \
  network_proxy.cc \
  paint_aggregator.cc \
  paint_manager.cc \
  ppp_entrypoints.cc \
  printing_dev.cc \
  rect.cc \
  resource.cc \
  resource_array_dev.cc \
  scriptable_object_deprecated.cc \
  scrollbar_dev.cc \
  selection_dev.cc \
  simple_thread.cc \
  socket_dev.cc \
  tcp_socket.cc \
  text_input_controller.cc \
  text_input_dev.cc \
  truetype_font_dev.cc \
  udp_socket.cc \
  url_loader.cc \
  url_request_info.cc \
  url_response_info.cc \
  url_util_dev.cc \
  var.cc \
  var_array.cc \
  var_array_buffer.cc \
  var_dictionary.cc \
  video_capture_client_dev.cc \
  video_capture_dev.cc \
  video_decoder_client_dev.cc \
  video_decoder_dev.cc \
  view.cc \
  view_dev.cc \
  websocket.cc \
  websocket_api.cc \
  widget_client_dev.cc \
  widget_dev.cc \
  zoom_dev.cc \

SOURCES:=$(foreach src,$(SOURCES),$(NACL_SDK_ROOT)/src/ppapi_cpp/$(src))

#
# Use the compile macro for each source.
#
$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),)))

#
# Use the lib macro for this target on the list of sources.
#
$(eval $(call LIB_RULE,ppapi_cpp,$(SOURCES)))
