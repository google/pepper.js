# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#
# GNU Make based build file.  For details on GNU Make see:
#   http://www.gnu.org/software/make/manual/make.html
#


#
# Default library paths
#

LD_X86_32:=-L$(LIBDIR)/$(TOOLCHAIN)_x86_32/$(CONFIG)
LD_X86_64:=-L$(LIBDIR)/$(TOOLCHAIN)_x86_64/$(CONFIG)
LD_ARM:=-L$(LIBDIR)/$(TOOLCHAIN)_arm/$(CONFIG)


#
# Macros for TOOLS
#
# We always link with the C++ compiler but include -Wl,-as-needed flag
# in LD_FLAGS so the linker should drop libc++ unless it's actually needed.
#
X86_TC_BIN ?= $(TC_PATH)/$(OSNAME)_x86_$(TOOLCHAIN)/bin
ARM_TC_BIN ?= $(TC_PATH)/$(OSNAME)_arm_$(TOOLCHAIN)/bin

X86_32_CC ?= $(NACL_COMPILER_PREFIX) $(X86_TC_BIN)/i686-nacl-gcc
X86_32_CXX ?= $(NACL_COMPILER_PREFIX) $(X86_TC_BIN)/i686-nacl-g++
X86_32_LINK ?= $(X86_TC_BIN)/i686-nacl-g++
X86_32_LIB ?= $(X86_TC_BIN)/i686-nacl-ar
X86_32_STRIP ?= $(X86_TC_BIN)/i686-nacl-strip
X86_32_NM ?= $(X86_TC_BIN)/i686-nacl-nm

X86_64_CC ?= $(NACL_COMPILER_PREFIX) $(X86_TC_BIN)/x86_64-nacl-gcc
X86_64_CXX ?= $(NACL_COMPILER_PREFIX) $(X86_TC_BIN)/x86_64-nacl-g++
X86_64_LINK ?= $(X86_TC_BIN)/x86_64-nacl-g++
X86_64_LIB ?= $(X86_TC_BIN)/x86_64-nacl-ar
X86_64_STRIP ?= $(X86_TC_BIN)/x86_64-nacl-strip
X86_64_NM ?= $(X86_TC_BIN)/x86_64-nacl-nm

ARM_CC ?= $(NACL_COMPILER_PREFIX) $(ARM_TC_BIN)/arm-nacl-gcc
ARM_CXX ?= $(NACL_COMPILER_PREFIX) $(ARM_TC_BIN)/arm-nacl-g++
ARM_LINK ?= $(ARM_TC_BIN)/arm-nacl-g++
ARM_LIB ?= $(ARM_TC_BIN)/arm-nacl-ar
ARM_STRIP ?= $(ARM_TC_BIN)/arm-nacl-strip
ARM_NM ?= $(ARM_TC_BIN)/arm-nacl-nm

# Architecture-specific flags
X86_32_CFLAGS ?=
X86_64_CFLAGS ?=
ARM_CFLAGS ?=

X86_32_CXXFLAGS ?=
X86_64_CXXFLAGS ?=
ARM_CXXFLAGS ?=

X86_32_LDFLAGS ?= -Wl,-Map,$(OUTDIR)/$(TARGET)_x86_32.map
X86_64_LDFLAGS ?= -Wl,-Map,$(OUTDIR)/$(TARGET)_x86_64.map
ARM_LDFLAGS ?= -Wl,-Map,$(OUTDIR)/$(TARGET)_arm.map

#
# Compile Macro
#
# $1 = Source Name
# $2 = Compile Flags
#
define C_COMPILER_RULE
-include $(call SRC_TO_DEP,$(1),_x86_32)
$(call SRC_TO_OBJ,$(1),_x86_32): $(1) $(TOP_MAKE) | $(dir $(call SRC_TO_OBJ,$(1)))dir.stamp
	$(call LOG,CC  ,$$@,$(X86_32_CC) -o $$@ -c $$< $(POSIX_FLAGS) $(2) $(NACL_CFLAGS) $(X86_32_CFLAGS))
	@$(FIXDEPS) $(call SRC_TO_DEP_PRE_FIXUP,$(1),_x86_32)

-include $(call SRC_TO_DEP,$(1),_x86_64)
$(call SRC_TO_OBJ,$(1),_x86_64): $(1) $(TOP_MAKE) | $(dir $(call SRC_TO_OBJ,$(1)))dir.stamp
	$(call LOG,CC  ,$$@,$(X86_64_CC) -o $$@ -c $$< $(POSIX_FLAGS) $(2) $(NACL_CFLAGS) $(X86_64_CFLAGS))
	@$(FIXDEPS) $(call SRC_TO_DEP_PRE_FIXUP,$(1),_x86_64)

-include $(call SRC_TO_DEP,$(1),_arm)
$(call SRC_TO_OBJ,$(1),_arm): $(1) $(TOP_MAKE) | $(dir $(call SRC_TO_OBJ,$(1)))dir.stamp
	$(call LOG,CC  ,$$@,$(ARM_CC) -o $$@ -c $$< $(POSIX_FLAGS) $(2) $(NACL_CFLAGS) $(ARM_CFLAGS))
	@$(FIXDEPS) $(call SRC_TO_DEP_PRE_FIXUP,$(1),_arm)

-include $(call SRC_TO_DEP,$(1),_x86_32_pic)
$(call SRC_TO_OBJ,$(1),_x86_32_pic): $(1) $(TOP_MAKE) | $(dir $(call SRC_TO_OBJ,$(1)))dir.stamp
	$(call LOG,CC  ,$$@,$(X86_32_CC) -o $$@ -c $$< -fPIC $(POSIX_FLAGS) $(2) $(NACL_CFLAGS) $(X86_32_CFLAGS))
	@$(FIXDEPS) $(call SRC_TO_DEP_PRE_FIXUP,$(1),_x86_32_pic)

-include $(call SRC_TO_DEP,$(1),_x86_64_pic)
$(call SRC_TO_OBJ,$(1),_x86_64_pic): $(1) $(TOP_MAKE) | $(dir $(call SRC_TO_OBJ,$(1)))dir.stamp
	$(call LOG,CC  ,$$@,$(X86_64_CC) -o $$@ -c $$< -fPIC $(POSIX_FLAGS) $(2) $(NACL_CFLAGS) $(X86_64_CFLAGS))
	@$(FIXDEPS) $(call SRC_TO_DEP_PRE_FIXUP,$(1),_x86_64_pic)

-include $(call SRC_TO_DEP,$(1),_arm_pic)
$(call SRC_TO_OBJ,$(1),_arm_pic): $(1) $(TOP_MAKE) | $(dir $(call SRC_TO_OBJ,$(1)))dir.stamp
	$(call LOG,CC  ,$$@,$(ARM_CC) -o $$@ -c $$< -fPIC $(POSIX_FLAGS) $(2) $(NACL_CFLAGS) $(ARM_CFLAGS))
	@$(FIXDEPS) $(call SRC_TO_DEP_PRE_FIXUP,$(1),_arm_pic)
endef

define CXX_COMPILER_RULE
-include $(call SRC_TO_DEP,$(1),_x86_32)
$(call SRC_TO_OBJ,$(1),_x86_32): $(1) $(TOP_MAKE) | $(dir $(call SRC_TO_OBJ,$(1)))dir.stamp
	$(call LOG,CXX ,$$@,$(X86_32_CXX) -o $$@ -c $$< $(POSIX_FLAGS) $(2) $(NACL_CXXFLAGS) $(X86_32_CXXFLAGS))
	@$(FIXDEPS) $(call SRC_TO_DEP_PRE_FIXUP,$(1),_x86_32)

-include $(call SRC_TO_DEP,$(1),_x86_64)
$(call SRC_TO_OBJ,$(1),_x86_64): $(1) $(TOP_MAKE) | $(dir $(call SRC_TO_OBJ,$(1)))dir.stamp
	$(call LOG,CXX ,$$@,$(X86_64_CXX) -o $$@ -c $$< $(POSIX_FLAGS) $(2) $(NACL_CXXFLAGS) $(X86_64_CXXFLAGS))
	@$(FIXDEPS) $(call SRC_TO_DEP_PRE_FIXUP,$(1),_x86_64)

-include $(call SRC_TO_DEP,$(1),_arm)
$(call SRC_TO_OBJ,$(1),_arm): $(1) $(TOP_MAKE) | $(dir $(call SRC_TO_OBJ,$(1)))dir.stamp
	$(call LOG,CXX ,$$@,$(ARM_CXX) -o $$@ -c $$< $(POSIX_FLAGS) $(2) $(NACL_CXXFLAGS) $(ARM_CXXFLAGS))
	@$(FIXDEPS) $(call SRC_TO_DEP_PRE_FIXUP,$(1),_arm)

-include $(call SRC_TO_DEP,$(1),_x86_32_pic)
$(call SRC_TO_OBJ,$(1),_x86_32_pic): $(1) $(TOP_MAKE) | $(dir $(call SRC_TO_OBJ,$(1)))dir.stamp
	$(call LOG,CXX ,$$@,$(X86_32_CXX) -o $$@ -c $$< -fPIC $(POSIX_FLAGS) $(2) $(NACL_CXXFLAGS) $(X86_32_CXXFLAGS))
	@$(FIXDEPS) $(call SRC_TO_DEP_PRE_FIXUP,$(1),_x86_32_pic)

-include $(call SRC_TO_DEP,$(1),_x86_64_pic)
$(call SRC_TO_OBJ,$(1),_x86_64_pic): $(1) $(TOP_MAKE) | $(dir $(call SRC_TO_OBJ,$(1)))dir.stamp
	$(call LOG,CXX ,$$@,$(X86_64_CXX) -o $$@ -c $$< -fPIC $(POSIX_FLAGS) $(2) $(NACL_CXXFLAGS) $(X86_64_CXXFLAGS))
	@$(FIXDEPS) $(call SRC_TO_DEP_PRE_FIXUP,$(1),_x86_64_pic)

-include $(call SRC_TO_DEP,$(1),_arm_pic)
$(call SRC_TO_OBJ,$(1),_arm_pic): $(1) $(TOP_MAKE) | $(dir $(call SRC_TO_OBJ,$(1)))dir.stamp
	$(call LOG,CXX ,$$@,$(ARM_CXX) -o $$@ -c $$< -fPIC $(POSIX_FLAGS) $(2) $(NACL_CXXFLAGS) $(ARM_CXXFLAGS))
	@$(FIXDEPS) $(call SRC_TO_DEP_PRE_FIXUP,$(1),_arm_pic)
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
# Determine which architectures to build for.  The user can set NACL_ARCH or
# ARCHES in the environment to control this.
#
VALID_ARCHES := x86_32 x86_64
ifeq (newlib,$(TOOLCHAIN))
VALID_ARCHES += arm
endif

ifdef NACL_ARCH
ifeq (,$(findstring $(NACL_ARCH),$(VALID_ARCHES)))
$(error Invalid arch specified in NACL_ARCH: $(NACL_ARCH).  Valid values are: $(VALID_ARCHES))
endif
ARCHES = ${NACL_ARCH}
else
ARCHES ?= ${VALID_ARCHES}
endif

GLIBC_REMAP :=

#
# SO Macro
#
# As well as building and installing a shared library this rule adds dependencies
# on the library's .stamp file in STAMPDIR.  However, the rule for creating the stamp
# file is part of LIB_RULE, so users of the DEPS system are currently required to
# use the LIB_RULE macro as well as the SO_RULE for each shared library.
#
# $1 = Target Name
# $2 = List of Sources
# $3 = List of LIBS
# $4 = List of DEPS
# $5 = 1 => Don't add to NMF.
#
define SO_RULE
ifneq (,$(findstring x86_32,$(ARCHES)))
all: $(OUTDIR)/lib$(1)_x86_32.so
$(OUTDIR)/lib$(1)_x86_32.so: $(foreach src,$(2),$(call SRC_TO_OBJ,$(src),_x86_32_pic)) $(4)
	$(call LOG,LINK,$$@,$(X86_32_LINK) -o $$@ $$(filter-out $(4),$$^) -shared -m32 $(LD_X86_32) $$(LD_FLAGS) $(foreach lib,$(3),-l$(lib)))

$(STAMPDIR)/$(1).stamp: $(LIBDIR)/$(TOOLCHAIN)_x86_32/$(CONFIG)/lib$(1).so
install: $(LIBDIR)/$(TOOLCHAIN)_x86_32/$(CONFIG)/lib$(1).so
$(LIBDIR)/$(TOOLCHAIN)_x86_32/$(CONFIG)/lib$(1).so: $(OUTDIR)/lib$(1)_x86_32.so
	$(MKDIR) -p $$(dir $$@)
	$(call LOG,CP  ,$$@,$(OSHELPERS) cp $$^ $$@)
ifneq ($(5),1)
GLIBC_SO_LIST += $(OUTDIR)/lib$(1)_x86_32.so
GLIBC_REMAP += -n lib$(1)_x86_32.so,lib$(1).so
endif
endif

ifneq (,$(findstring x86_64,$(ARCHES)))
all: $(OUTDIR)/lib$(1)_x86_64.so
$(OUTDIR)/lib$(1)_x86_64.so: $(foreach src,$(2),$(call SRC_TO_OBJ,$(src),_x86_64_pic)) $(4)
	$(call LOG,LINK,$$@,$(X86_32_LINK) -o $$@ $$(filter-out $(4),$$^) -shared -m64 $(LD_X86_64) $$(LD_FLAGS) $(foreach lib,$(3),-l$(lib)))

$(STAMPDIR)/$(1).stamp: $(LIBDIR)/$(TOOLCHAIN)_x86_64/$(CONFIG)/lib$(1).so
install: $(LIBDIR)/$(TOOLCHAIN)_x86_64/$(CONFIG)/lib$(1).so
$(LIBDIR)/$(TOOLCHAIN)_x86_64/$(CONFIG)/lib$(1).so: $(OUTDIR)/lib$(1)_x86_64.so
	$(MKDIR) -p $$(dir $$@)
	$(call LOG,CP  ,$$@,$(OSHELPERS) cp $$^ $$@)
ifneq ($(5),1)
GLIBC_SO_LIST += $(OUTDIR)/lib$(1)_x86_64.so
GLIBC_REMAP += -n lib$(1)_x86_64.so,lib$(1).so
endif
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
$(STAMPDIR)/$(1).stamp:
	@echo "  STAMP $$@"
	@echo "TOUCHED $$@" > $(STAMPDIR)/$(1).stamp

ifneq (,$(findstring x86_32,$(ARCHES)))
all: $(OUTDIR)/lib$(1)_x86_32.a
$(OUTDIR)/lib$(1)_x86_32.a: $(foreach src,$(2),$(call SRC_TO_OBJ,$(src),_x86_32))
	$(MKDIR) -p $$(dir $$@)
	$(call LOG,LIB ,$$@,$(X86_32_LIB) -cr $$@ $$^)

$(STAMPDIR)/$(1).stamp: $(LIBDIR)/$(TOOLCHAIN)_x86_32/$(CONFIG)/lib$(1).a
install: $(LIBDIR)/$(TOOLCHAIN)_x86_32/$(CONFIG)/lib$(1).a
$(LIBDIR)/$(TOOLCHAIN)_x86_32/$(CONFIG)/lib$(1).a: $(OUTDIR)/lib$(1)_x86_32.a
	$(MKDIR) -p $$(dir $$@)
	$(call LOG,CP  ,$$@,$(OSHELPERS) cp $$^ $$@)
endif

ifneq (,$(findstring x86_64,$(ARCHES)))
all: $(OUTDIR)/lib$(1)_x86_64.a
$(OUTDIR)/lib$(1)_x86_64.a: $(foreach src,$(2),$(call SRC_TO_OBJ,$(src),_x86_64))
	$(MKDIR) -p $$(dir $$@)
	$(call LOG,LIB ,$$@,$(X86_64_LIB) -cr $$@ $$^)

$(STAMPDIR)/$(1).stamp: $(LIBDIR)/$(TOOLCHAIN)_x86_64/$(CONFIG)/lib$(1).a
install: $(LIBDIR)/$(TOOLCHAIN)_x86_64/$(CONFIG)/lib$(1).a
$(LIBDIR)/$(TOOLCHAIN)_x86_64/$(CONFIG)/lib$(1).a: $(OUTDIR)/lib$(1)_x86_64.a
	$(MKDIR) -p $$(dir $$@)
	$(call LOG,CP  ,$$@,$(OSHELPERS) cp $$^ $$@)
endif

ifneq (,$(findstring arm,$(ARCHES)))
ifneq ($(TOOLCHAIN),glibc)
all: $(OUTDIR)/lib$(1)_arm.a
$(OUTDIR)/lib$(1)_arm.a: $(foreach src,$(2),$(call SRC_TO_OBJ,$(src),_arm))
	$(MKDIR) -p $$(dir $$@)
	$(call LOG,LIB ,$$@,$(ARM_LIB) -cr $$@ $$^)

$(STAMPDIR)/$(1).stamp: $(LIBDIR)/$(TOOLCHAIN)_arm/$(CONFIG)/lib$(1).a
install: $(LIBDIR)/$(TOOLCHAIN)_arm/$(CONFIG)/lib$(1).a
$(LIBDIR)/$(TOOLCHAIN)_arm/$(CONFIG)/lib$(1).a: $(OUTDIR)/lib$(1)_arm.a
	$(MKDIR) -p $$(dir $$@)
	$(call LOG,CP  ,$$@,$(OSHELPERS) cp $$^ $$@)
endif
endif
endef


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
ifneq (,$(findstring x86_32,$(ARCHES)))
all: $(OUTDIR)/$(1)_x86_32.nexe
$(OUTDIR)/$(1)_x86_32.nexe: $(foreach src,$(2),$(call SRC_TO_OBJ,$(src),_x86_32)) $(foreach dep,$(4),$(STAMPDIR)/$(dep).stamp)
	$(call LOG,LINK,$$@,$(X86_32_LINK) -o $$@ $$(filter %.o,$$^) $(NACL_LDFLAGS) $(X86_32_LDFLAGS) $(foreach path,$(6),-L$(path)/$(TOOLCHAIN)_x86_32/$(CONFIG)) $(foreach lib,$(3),-l$(lib)) $(5))
endif

ifneq (,$(findstring x86_64,$(ARCHES)))
all: $(OUTDIR)/$(1)_x86_64.nexe
$(OUTDIR)/$(1)_x86_64.nexe: $(foreach src,$(2),$(call SRC_TO_OBJ,$(src),_x86_64)) $(foreach dep,$(4),$(STAMPDIR)/$(dep).stamp)
	$(call LOG,LINK,$$@,$(X86_64_LINK) -o $$@ $$(filter %.o,$$^) $(NACL_LDFLAGS) $(X86_64_LDFLAGS) $(foreach path,$(6),-L$(path)/$(TOOLCHAIN)_x86_64/$(CONFIG)) $(foreach lib,$(3),-l$(lib)) $(5))
endif

ifneq (,$(findstring arm,$(ARCHES)))
all: $(OUTDIR)/$(1)_arm.nexe
$(OUTDIR)/$(1)_arm.nexe: $(foreach src,$(2),$(call SRC_TO_OBJ,$(src),_arm)) $(foreach dep,$(4),$(STAMPDIR)/$(dep).stamp)
	$(call LOG,LINK,$$@,$(ARM_LINK) -o $$@ $$(filter %.o,$$^) $(NACL_LDFLAGS) $(ARM_LDFLAGS) $(foreach path,$(6),-L$(path)/$(TOOLCHAIN)_arm/$(CONFIG)) $(foreach lib,$(3),-l$(lib)) $(5))
endif
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
ifeq ($(CONFIG),Release)
$(call LINKER_RULE,$(1)_unstripped,$(2),$(filter-out pthread,$(3)),$(4),$(5),$(LIB_PATHS))
$(call STRIP_ALL_RULE,$(1),$(1)_unstripped)
else
$(call LINKER_RULE,$(1),$(2),$(filter-out pthread,$(3)),$(4),$(5),$(LIB_PATHS))
endif
endef


#
# Strip Macro for each arch (e.g., each arch supported by LINKER_RULE).
#
# $1 = Target Name
# $2 = Source Name
#
define STRIP_ALL_RULE
ifneq (,$(findstring x86_32,$(ARCHES)))
$(OUTDIR)/$(1)_x86_32.nexe: $(OUTDIR)/$(2)_x86_32.nexe
	$(call LOG,STRIP,$$@,$(X86_32_STRIP) -s -o $$@ $$^)
endif

ifneq (,$(findstring x86_64,$(ARCHES)))
$(OUTDIR)/$(1)_x86_64.nexe: $(OUTDIR)/$(2)_x86_64.nexe
	$(call LOG,STRIP,$$@,$(X86_64_STRIP) -s -o $$@ $$^)
endif

ifneq (,$(findstring arm,$(ARCHES)))
$(OUTDIR)/$(1)_arm.nexe: $(OUTDIR)/$(2)_arm.nexe
	$(call LOG,STRIP,$$@,$(ARM_STRIP) -s -o $$@ $$^)
endif
endef


#
# Top-level Strip Macro
#
# $1 = Target Basename
# $2 = Source Basename
#
define STRIP_RULE
$(call STRIP_ALL_RULE,$(1),$(2))
endef


#
# Strip Macro for each arch (e.g., each arch supported by MAP_RULE).
#
# $1 = Target Name
# $2 = Source Name
#
define MAP_ALL_RULE
ifneq (,$(findstring x86_32,$(ARCHES)))
all: $(OUTDIR)/$(1)_x86_32.map
$(OUTDIR)/$(1)_x86_32.map: $(OUTDIR)/$(2)_x86_32.nexe
	$(call LOG,MAP,$$@,$(X86_32_NM) -l $$^ > $$@)
endif

ifneq (,$(findstring x86_64,$(ARCHES)))
all: $(OUTDIR)/$(1)_x86_64.map
$(OUTDIR)/$(1)_x86_64.map: $(OUTDIR)/$(2)_x86_64.nexe
	$(call LOG,MAP,$$@,$(X86_64_NM) -l $$^ > $$@)
endif

ifneq (,$(findstring arm,$(ARCHES)))
all: $(OUTDIR)/$(1)_arm.map
$(OUTDIR)/$(1)_arm.map: $(OUTDIR)/$(2)_arm.nexe
	$(call LOG,MAP,$$@,$(ARM_NM) -l $$^ > $$@ )
endif
endef


#
# Top-level MAP Generation Macro
#
# $1 = Target Basename
# $2 = Source Basename
#
define MAP_RULE
$(call MAP_ALL_RULE,$(1),$(2))
endef


#
# Generate ARCH_SUFFIXES, a list of suffixes for executables corresponding to all
# the architectures in the current build.
#
ARCH_SUFFIXES := $(foreach arch,$(ARCHES),_$(arch).nexe)


#
# NMF Manifiest generation
#
# Use the python script create_nmf to scan the binaries for dependencies using
# objdump.  Pass in the (-L) paths to the default library toolchains so that we
# can find those libraries and have it automatically copy the files (-s) to
# the target directory for us.
#
# $1 = Target Name (the basename of the nmf
# $2 = Additional create_nmf.py arguments
#
NMF := python $(NACL_SDK_ROOT)/tools/create_nmf.py
ifeq ($(CONFIG),Debug)
NMF_FLAGS += --debug-libs
HTML_FLAGS += --debug-libs
endif

EXECUTABLES=$(foreach arch,$(ARCH_SUFFIXES),$(OUTDIR)/$(1)$(arch)) $(GLIBC_SO_LIST)

define NMF_RULE
all: $(OUTDIR)/$(1).nmf
$(OUTDIR)/$(1).nmf: $(EXECUTABLES)
	$(call LOG,CREATE_NMF,$$@,$(NMF) $(NMF_FLAGS) -o $$@ $$^ $(GLIBC_PATHS) -s $(OUTDIR) $(2) $(GLIBC_REMAP))
endef

#
# HTML file generation
#
CREATE_HTML := python $(NACL_SDK_ROOT)/tools/create_html.py

define HTML_RULE
all: $(OUTDIR)/$(1).html
$(OUTDIR)/$(1).html: $(EXECUTABLES)
	$(call LOG,CREATE_HTML,$$@,$(CREATE_HTML) $(HTML_FLAGS) -o $$@ $$^)
endef
