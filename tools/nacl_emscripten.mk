# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#
# GNU Make based build file.  For details on GNU Make see:
#   http://www.gnu.org/software/make/manual/make.html
#
#


#
# Default Paths
#
EM_PATH:=$(abspath $(DEPLUG_SRC_ROOT)/../emscripten)
#$(error $(EM_PATH))

LIBDIR:=$(DEPLUG_SRC_ROOT)/lib/emscripten/$(CONFIG)

LD_X86_32:=-L$(DEPLUG_SRC_ROOT)/lib/emscripten/$(CONFIG)

#
# Macros for TOOLS
#
EM_CC?=$(EM_PATH)/emcc
EM_CXX?=$(EM_PATH)/em++
EM_LINK?=$(EM_PATH)/em++
EM_LIB?=$(EM_PATH)/emar

# Architecture-specific flags
EM_CFLAGS?=-DNACL_ARCH=x86_32
EM_CXXFLAGS?=-DNACL_ARCH=x86_32

# Emscripten currently has a warning about -nostdinc++, prevent this from causing an error.
NACL_CFLAGS:=$(filter-out -Werror,$(NACL_CFLAGS))
NACL_CXXFLAGS:=$(filter-out -Werror,$(NACL_CXXFLAGS))

NACL_CFLAGS?=-Wno-long-long -Werror
NACL_CXXFLAGS?=-Wno-long-long -Werror
NACL_LDFLAGS?=-Wl,-as-needed

# Emscripten appears to key off the optimization while linking to deterimine what backend should be used.
# HACK PPAPI will export a bunch of anonymous function pointers, so we need to reserve slots for them.
# Eventually we should make these non-anonymous.
ifeq ($(CONFIG),Release)
NACL_LDFLAGS+=-O2 -s RESERVED_FUNCTION_POINTERS=1000
else
NACL_LDFLAGS+=-O0
endif

# No threads.
POSIX_FLAGS:=$(filter-out -pthread,$(POSIX_FLAGS))
#
# Compile Macro
#
# $1 = Source Name
# $2 = Compile Flags
#
define C_COMPILER_RULE
-include $(call SRC_TO_DEP,$(1),_emscripten)
$(call SRC_TO_OBJ,$(1),_emscripten): $(1) $(TOP_MAKE) | $(dir $(call SRC_TO_OBJ,$(1)))dir.stamp
	$(call LOG,CC,$$@,$(EM_CC) -o $$@ -c $$< $(POSIX_FLAGS) $(2) $(NACL_CFLAGS) $(EM_CFLAGS))
endef

define CXX_COMPILER_RULE
-include $(call SRC_TO_DEP,$(1),_emscripten)
$(call SRC_TO_OBJ,$(1),_emscripten): $(1) $(TOP_MAKE) | $(dir $(call SRC_TO_OBJ,$(1)))dir.stamp
	$(call LOG,CXX,$$@,$(EM_CXX) -o $$@ -c $$< $(POSIX_FLAGS) $(2) $(NACL_CXXFLAGS) $(EM_CXXFLAGS))
endef


#
# $1 = Source Name
# $2 = POSIX Compile Flags
# $3 = Include Directories
# $4 = VC Flags (unused)
#
define COMPILE_RULE
ifeq ($(suffix $(1)),.c)
$(call C_COMPILER_RULE,$(1),$(2) $(foreach inc,$(INC_PATHS),-I$(inc)) $(3))
else
$(call CXX_COMPILER_RULE,$(1),$(2) $(foreach inc,$(INC_PATHS),-I$(inc)) $(3))
endif
endef

#
# LIB Macro
#
# $1 = Target Name
# $2 = List of Sources
# $3 = POSIX Link Flags
# $4 = VC Link Flags (unused)
#
define LIB_RULE
$(STAMPDIR)/$(1).stamp : $(LIBDIR)/lib$(1).a

$(STAMPDIR)/$(1).stamp :
	@echo "TOUCHED $$@" > $(STAMPDIR)/$(1).stamp

all: $(LIBDIR)/lib$(1).a
$(LIBDIR)/lib$(1).a : $(foreach src,$(2),$(call SRC_TO_OBJ,$(src),_emscripten))
	$(MKDIR) -p $$(dir $$@)
	$(call LOG,LIB,$$@,$(EM_LIB) r $$@ $$^)

endef

JS_LIBRARIES=$(DEPLUG_SRC_ROOT)/library_ppapi.js
JS_PRE=$(DEPLUG_SRC_ROOT)/deplug.js $(DEPLUG_SRC_ROOT)/ppapi.js


# TODO(ncbray): only include needed wrappers.
WRAPPERS=base.js url_loader.js graphics_2d.js view.js graphics_3d.js gles.js gles_ext.js
# Resolve the paths.
WRAPPERS:=$(foreach wrapper,$(WRAPPERS),$(DEPLUG_SRC_ROOT)/wrappers/$(wrapper))

#
# Specific Link Macro
#
# $1 = Target Name
# $2 = List of Sources
# $3 = List of LIBS
# $4 = List of DEPS
# $5 = POSIX Link Flags
# $6 = Library Paths
#
define LINKER_RULE
all: $(OUTDIR)/$(1).js
$(OUTDIR)/$(1).js : $(foreach src,$(2),$(call SRC_TO_OBJ,$(src),_emscripten)) $(foreach dep,$(4),$(STAMPDIR)/$(dep).stamp) $(JS_LIBRARIES) $(JS_PRE) $(WRAPPERS)
	$(call LOG,LINK,$$@,$(EM_LINK) -o $$@ $$(filter %.o,$$^) $(NACL_LDFLAGS) $(foreach path,$(6),-L$(path)/emscripten/$(CONFIG)) $(foreach lib,$(3),-l$(lib)) $(foreach file,$(JS_LIBRARIES),--js-library $(file)) $(foreach file,$(JS_PRE),--pre-js $(file)) $(foreach wrapper,$(WRAPPERS),--post-js $(wrapper)) $(5))
endef

#
# Generalized Link Macro
#
# $1 = Target Name
# $2 = List of Sources
# $3 = List of LIBS
# $4 = List of DEPS
# $5 = POSIX Linker Switches
# $6 = VC Linker Switches
#
define LINK_RULE
$(call LINKER_RULE,$(1),$(2),$(filter-out pthread ppapi,$(3)),$(4),$(5),$(LIB_PATHS))
endef


#
# Top-level Strip Macro
#
# $1 = Target Basename
# $2 = Source Basename
#
# For now this is just a no-op
define STRIP_RULE
$(OUTDIR)/$(1).js : $(OUTDIR)/$(2).js
	cp $(OUTDIR)/$(2).js $(OUTDIR)/$(1).js
all: $(OUTDIR)/$(1).js
endef
