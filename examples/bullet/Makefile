# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# GNU Makefile based on shared rules provided by the Native Client SDK.
# See README.Makefiles for more details.

# Top-level Makefile that builds the bullet library and an example that uses
# it.

VALID_TOOLCHAINS := emscripten pnacl

PEPPERJS_SRC_ROOT?=../..
include $(PEPPERJS_SRC_ROOT)/tools/common.mk

ifeq (win,$(OSNAME))
  $(error "This example cannot be built on Windows.")
endif

all:
	@./build.sh $(TOOLCHAIN) $(CONFIG)
