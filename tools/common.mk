# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#
# GNU Make based build file.  For details on GNU Make see:
#   http://www.gnu.org/software/make/manual/make.html
#

#
# Toolchain
#
# By default the VALID_TOOLCHAINS list contains newlib and glibc.  If your
# project only builds in one or the other then this should be overridden
# accordingly.
#
VALID_TOOLCHAINS?=newlib glibc pnacl emscripten
TOOLCHAIN?=$(word 1,$(VALID_TOOLCHAINS))


#
# Top Make file, which we want to trigger a rebuild on if it changes
#
TOP_MAKE:=$(word 1,$(MAKEFILE_LIST))

#
# Verify we selected a valid toolchain for this example
#
ifeq (,$(findstring $(TOOLCHAIN),$(VALID_TOOLCHAINS)))
  $(warning Availbile choices are: $(VALID_TOOLCHAINS))
  $(error Can not use TOOLCHAIN=$(TOOLCHAIN) on this example.)
endif

#
# Build Configuration
#
# The SDK provides two sets of libraries, Debug and Release.  Debug libraries
# are compiled without optimizations to make debugging easier.  By default
# this will build a Debug configuration.
#
CONFIG ?= Debug


#
# Note for Windows:
#   The GCC and LLVM toolchains (include the version of Make.exe that comes
# with the SDK) expect and are capable of dealing with the '/' seperator.
# For this reason, the tools in the SDK, including Makefiles and build scripts
# have a preference for POSIX style command-line arguments.
#
# Keep in mind however that the shell is responsible for command-line escaping,
# globbing, and variable expansion, so those may change based on which shell
# is used.  For Cygwin shells this can include automatic and incorrect expansion
# of response files (files starting with '@').
#
# Disable DOS PATH warning when using Cygwin based NaCl tools on Windows.
#
ifeq ($(OSNAME),win)
  CYGWIN?=nodosfilewarning
  export CYGWIN
endif


ifndef NACL_SDK_ROOT
$(error "The NACL_SDK_ROOT environment variable must be set.  pepper_30 and newer are supported.")
endif

#
# Check that NACL_SDK_ROOT is set to a valid location.
# We use the existence of tools/oshelpers.py to verify the validity of the SDK
# root.
#
ifeq (,$(wildcard $(NACL_SDK_ROOT)/tools/oshelpers.py))
  $(error NACL_SDK_ROOT is set to an invalid location: $(NACL_SDK_ROOT))
endif

NACL_CONFIG := python $(NACL_SDK_ROOT)/tools/nacl_config.py

#
# If this makefile is part of a valid nacl SDK, but NACL_SDK_ROOT is set
# to a different location this is almost certainly a local configuration
# error.
#
LOCAL_ROOT := $(realpath $(dir $(THIS_MAKEFILE))/..)
ifneq (,$(wildcard $(LOCAL_ROOT)/tools/oshelpers.py))
  ifneq ($(realpath $(NACL_SDK_ROOT)), $(realpath $(LOCAL_ROOT)))
    $(error common.mk included from an SDK that does not match the current NACL_SDK_ROOT)
  endif
endif


#
# Alias for standard POSIX file system commands
#
OSHELPERS = python $(NACL_SDK_ROOT)/tools/oshelpers.py
WHICH := $(OSHELPERS) which
ifdef V
RM := $(OSHELPERS) rm
CP := $(OSHELPERS) cp
MKDIR := $(OSHELPERS) mkdir
MV := $(OSHELPERS) mv
else
RM := @$(OSHELPERS) rm
CP := @$(OSHELPERS) cp
MKDIR := @$(OSHELPERS) mkdir
MV := @$(OSHELPERS) mv
endif



#
# Compute path to requested NaCl Toolchain
#
GETOS=python $(NACL_SDK_ROOT)/tools/getos.py
OSNAME:=$(shell $(GETOS))
TC_PATH:=$(abspath $(NACL_SDK_ROOT)/toolchain)


#
# Check for required minimum SDK version.
#
ifdef NACL_SDK_VERSION_MIN
  VERSION_CHECK:=$(shell $(GETOS) --check-version=$(NACL_SDK_VERSION_MIN) 2>&1)
  ifneq ($(VERSION_CHECK),)
    $(error $(VERSION_CHECK))
  endif
endif


#
# The default target
#
# If no targets are specified on the command-line, the first target listed in
# the makefile becomes the default target.  By convention this is usually called
# the 'all' target.  Here we leave it blank to be first, but define it later
#
all:


#
# Target a toolchain
#
# $1 = Toolchain Name
#
define TOOLCHAIN_RULE
.PHONY: all_$(1)
all_$(1):
	+$(MAKE) TOOLCHAIN=$(1)
TOOLCHAIN_LIST+=all_$(1)
endef


#
# The target for all versions
#
USABLE_TOOLCHAINS=$(filter $(OSNAME) newlib glibc pnacl emscripten,$(VALID_TOOLCHAINS))

ifeq ($(NO_HOST_BUILDS),1)
USABLE_TOOLCHAINS:=$(filter-out $(OSNAME),$(USABLE_TOOLCHAINS))
endif

$(foreach tool,$(USABLE_TOOLCHAINS),$(eval $(call TOOLCHAIN_RULE,$(tool),$(dep))))

.PHONY: all_versions
all_versions: $(TOOLCHAIN_LIST)


OUTBASE?=.
OUTDIR:=$(OUTBASE)/$(TOOLCHAIN)/$(CONFIG)
STAMPDIR?=$(OUTDIR)


#
# Target to remove temporary files
#
.PHONY: clean
clean:
	$(RM) -f $(TARGET).nmf
	$(RM) -fr $(OUTDIR)


#
# Rules for output directories.
#
# Output will be places in a directory name based on Toolchain and configuration
# be default this will be "newlib/Debug".  We use a python wrapped MKDIR to
# proivde a cross platform solution. The use of '|' checks for existance instead
# of timestamp, since the directory can update when files change.
#
%dir.stamp :
	$(MKDIR) -p $(dir $@)
	@echo Directory Stamp > $@


#
# Dependency Macro
#
# $1 = Name of stamp
# $2 = Directory for the sub-make
# $3 = Extra Settings
#
define DEPEND_RULE
ifndef IGNORE_DEPS
.PHONY: rebuild_$(1)

rebuild_$(1) :| $(STAMPDIR)/dir.stamp
ifeq (,$(2))
	+$(MAKE) -C $(PEPPERJS_SRC_ROOT)/examples/$(1) STAMPDIR=$(abspath $(STAMPDIR)) $(abspath $(STAMPDIR)/$(1).stamp) $(3)
else
	+$(MAKE) -C $(2) STAMPDIR=$(abspath $(STAMPDIR)) $(abspath $(STAMPDIR)/$(1).stamp) $(3)
endif

all: rebuild_$(1)
$(STAMPDIR)/$(1).stamp: rebuild_$(1)

else

.PHONY: $(STAMPDIR)/$(1).stamp
$(STAMPDIR)/$(1).stamp:
	@echo Ignore $(1)
endif
endef


ifeq ($(TOOLCHAIN),win)
HOST_EXT = .dll
else
HOST_EXT = .so
endif


#
# Common Compile Options
#
ifeq ($(CONFIG),Release)
POSIX_FLAGS ?= -g -O2 -pthread -MMD
else
POSIX_FLAGS ?= -g -O0 -pthread -MMD -DNACL_SDK_DEBUG
endif

ifdef SEL_LDR
POSIX_FLAGS += -DSEL_LDR=1
endif

NACL_CFLAGS ?= -Wno-long-long -Werror
NACL_CXXFLAGS ?= -Wno-long-long -Werror
NACL_LDFLAGS ?= -Wl,-as-needed

#
# Default Paths
#
ifeq (,$(findstring $(TOOLCHAIN),linux mac win))
INC_PATHS?=$(NACL_SDK_ROOT)/include $(EXTRA_INC_PATHS)
else
INC_PATHS ?= $(NACL_SDK_ROOT)/include/$(OSNAME) $(NACL_SDK_ROOT)/include $(EXTRA_INC_PATHS)
endif

LIBDIR=$(PEPPERJS_SRC_ROOT)/lib
LIB_PATHS ?= $(LIBDIR) $(EXTRA_LIB_PATHS)

#
# Define a LOG macro that allow a command to be run in quiet mode where
# the command echoed is not the same as the actual command executed.
# The primary use case for this is to avoid echoing the full compiler
# and linker command in the default case.  Defining V=1 will restore
# the verbose behavior
#
# $1 = The name of the tool being run
# $2 = The target file being built
# $3 = The full command to run
#
ifdef V
define LOG
$(3)
endef
else
ifeq ($(OSNAME),win)
define LOG
@echo   $(1) $(2) && $(3)
endef
else
define LOG
@echo "  $(1) $(2)" && $(3)
endef
endif
endif


#
# Convert a source path to a object file path.
#
# $1 = Source Name
# $2 = Arch suffix
#
# HACK simplify name mangling.
#$(OUTDIR)/$(basename $(subst ..,__,$(1)))$(2).o
define SRC_TO_OBJ
$(OUTDIR)/$(basename $(notdir $(1)))$(2).o
endef


#
# Convert a source path to a dependency file path.
#
# $1 = Source Name
# $2 = Arch suffix
#
define SRC_TO_DEP
$(patsubst %.o,%.d,$(call SRC_TO_OBJ,$(1),$(2)))
endef


#
# If the requested toolchain is a NaCl or PNaCl toolchain, the use the
# macros and targets defined in nacl.mk, otherwise use the host sepecific
# macros and targets.
#
ifneq (,$(findstring $(TOOLCHAIN),linux mac))
include $(NACL_SDK_ROOT)/tools/host_gcc.mk
endif

ifneq (,$(findstring $(TOOLCHAIN),win))
include $(NACL_SDK_ROOT)/tools/host_vc.mk
endif

ifneq (,$(findstring $(TOOLCHAIN),glibc newlib))
include $(PEPPERJS_SRC_ROOT)/tools/nacl_gcc.mk
endif

ifneq (,$(findstring $(TOOLCHAIN),pnacl))
include $(NACL_SDK_ROOT)/tools/nacl_llvm.mk
endif

ifneq (,$(findstring $(TOOLCHAIN),emscripten))
include $(PEPPERJS_SRC_ROOT)/tools/nacl_emscripten.mk
endif


#
# File to redirect to to in order to hide output.
#
ifeq ($(OSNAME),win)
DEV_NULL = nul
else
DEV_NULL = /dev/null
endif

#
# Assign a sensible default to CHROME_PATH.
#
CHROME_PATH?=$(shell python $(NACL_SDK_ROOT)/tools/getos.py --chrome 2> $(DEV_NULL))

#
# Verify we can find the Chrome executable if we need to launch it.
#
CHECK_FOR_CHROME:
ifeq (,$(wildcard $(CHROME_PATH)))
	$(warning No valid Chrome found at CHROME_PATH=$(CHROME_PATH))
	$(error Set CHROME_PATH via an environment variable, or command-line.)
else
	$(warning Using chrome at: $(CHROME_PATH))
endif


#
# Variables for running examples with Chrome.
#
RUN_PY:=python $(NACL_SDK_ROOT)/tools/run.py

# Add this to launch Chrome with additional environment variables defined.
# Each element should be specified as KEY=VALUE, with whitespace separating
# key-value pairs. e.g.
# CHROME_ENV=FOO=1 BAR=2 BAZ=3
CHROME_ENV?=

# Additional arguments to pass to Chrome.
CHROME_ARGS+=--enable-nacl --enable-pnacl --incognito --ppapi-out-of-process


# Paths to Debug and Release versions of the Host Pepper plugins
PPAPI_DEBUG=$(abspath $(OSNAME)/Debug/$(TARGET)$(HOST_EXT));application/x-ppapi-debug
PPAPI_RELEASE=$(abspath $(OSNAME)/Release/$(TARGET)$(HOST_EXT));application/x-ppapi-release


PAGE?=index.html
PAGE_TC_CONFIG="$(PAGE)?tc=$(TOOLCHAIN)&config=$(CONFIG)"

RUN: LAUNCH
LAUNCH: CHECK_FOR_CHROME all
ifeq (,$(wildcard $(PAGE)))
	$(error No valid HTML page found at $(PAGE))
endif
	$(RUN_PY) -C $(CURDIR) -P $(PAGE_TC_CONFIG) \
	    $(addprefix -E ,$(CHROME_ENV)) -- $(CHROME_PATH) $(CHROME_ARGS) \
	    --register-pepper-plugins="$(PPAPI_DEBUG),$(PPAPI_RELEASE)"


SYSARCH=$(shell python $(NACL_SDK_ROOT)/tools/getos.py --nacl-arch)
GDB_ARGS+=-D $(TC_PATH)/$(OSNAME)_x86_$(TOOLCHAIN)/bin/$(SYSARCH)-nacl-gdb
GDB_ARGS+=-D $(abspath $(OUTDIR))/$(TARGET)_$(SYSARCH).nexe

DEBUG: CHECK_FOR_CHROME all
	$(RUN_PY) $(GDB_ARGS) \
	    -C $(CURDIR) -P $(PAGE_TC_CONFIG) \
	    $(addprefix -E ,$(CHROME_ENV)) -- $(CHROME_PATH) $(CHROME_ARGS) \
	    --enable-nacl-debug \
	    --register-pepper-plugins="$(PPAPI_DEBUG),$(PPAPI_RELEASE)"

.PHONY: CHECK_FOR_CHROME RUN LAUNCH
