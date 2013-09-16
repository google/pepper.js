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

# Really, this library shouldn't be built for newlib, but it makes the build simpler.

PEPPERJS_SRC_ROOT:=../..
EXTRA_INC_PATHS=
include $(PEPPERJS_SRC_ROOT)/tools/common.mk

#
# Target Name
#
# The base name of the final library, also the name of the NMF file containing
# the mapping between architecture and actual NEXE.
#
TARGET=ppapi

#
# List of sources to compile
#
SOURCES= \
  $(CURDIR)/stub.cc \
# Note: absolute path to help Emscripten source mapping.

ifneq (,$(findstring $(TOOLCHAIN),emscripten))
#
# Use the compile macro for each source.
#
$(foreach src,$(SOURCES),$(eval $(call COMPILE_RULE,$(src),)))

#
# Use the lib macro for this target on the list of sources.
#
$(eval $(call LIB_RULE,ppapi,$(SOURCES)))
else
$(STAMPDIR)/ppapi.stamp :
	@echo "TOUCHED $$@" > $(STAMPDIR)/ppapi.stamp
endif
