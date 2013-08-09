# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#
# GNU Make based build file.  For details on GNU Make see:
#   http://www.gnu.org/software/make/manual/make.html
#
#


#
# Default configuration
#
# By default we will build a Debug configuration using the GCC newlib toolcahin
# to override this, specify TOOLCHAIN=newlib|glibc or CONFIG=Debug|Release on
# the make command-line or in this file prior to including common.mk.  The
# toolchain we use by default will be the first valid one listed
VALID_TOOLCHAINS:=emscripten newlib pnacl




#
# Get pepper directory for toolchain and includes.
#
# If NACL_SDK_ROOT is not set, then assume it can be found relative to
# to this Makefile.
#
NACL_SDK_ROOT?=$(abspath $(CURDIR)/../../../../nacl_sdk/pepper_27)
PPAPIJS_SRC_ROOT:=../..
include $(PPAPIJS_SRC_ROOT)/tools/common.mk

#
# Target Name
#
# The base name of the final NEXE, also the name of the NMF file containing
# the mapping between architecture and actual NEXE.
#
TARGET=sine_synth

#
# List of sources to compile
#
sine_synth_SOURCES= \
  sine_synth.cc \

# TODO(ncbray): find a better way to inject the shim
ifeq (emscripten,$(TOOLCHAIN))
sine_synth_SOURCES := $(sine_synth_SOURCES) ../../deplug.cc
endif

#
# List of libraries to link against.  Unlike some tools, the GCC and LLVM
# based tools require libraries to be specified in the correct order.  The
# order should be symbol reference followed by symbol definition, with direct
# sources to the link (object files) are left most.  In this case:
#    hello_world -> ppapi_main -> ppapi_cpp -> ppapi -> pthread -> libc
# Notice that libc is implied and come last through standard compiler/link
# switches.
#
# We break this list down into two parts, the set we need to rebuild (DEPS)
# and the set we do not.  This example does not havea any additional library
# dependencies.
#
DEPS=ppapi_cpp
LIBS=$(DEPS) ppapi pthread


#
# Use the library dependency macro for each dependency
#
$(foreach dep,$(DEPS),$(eval $(call DEPEND_RULE,$(dep))))

#
# Use the compile macro for each source.
#
$(foreach src,$(sine_synth_SOURCES),$(eval $(call COMPILE_RULE,$(src),)))

#
# Use the link macro for this target on the list of sources.
#
$(eval $(call LINK_RULE,$(TARGET),$(sine_synth_SOURCES),$(LIBS),$(DEPS)))

#
# Specify the NMF to be created with no additional arugments.
#
$(eval $(call NMF_RULE,$(TARGET),))
