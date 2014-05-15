# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#
# GNU Make based build file.  For details on GNU Make see:
#   http://www.gnu.org/software/make/manual/make.html
#

PROJECTS:= \
  ppapi \
  ppapi_cpp \
  ppapi_gles2 \
  hello_world \
  geturl \
  pi_generator \
  hello_world_gles \
  input_events \
  mouselock \
  sine_synth \
  voronoi \
  earth \
  file_io \
  tests/ppapi/tests \
  tests/ppapi_messaging \
  tests/ppapi_geturl \
  tests/ppapi_websocket \
  tests/ppapi_vars \
  stress \
  probe_interfaces \

HTTPD:=../tools/httpd.py

# Define the default target
all:

#
# Target Macro
#
# Macro defines a phony target for each example, and adds it to a list of
# targets.
#
define TARGET
TARGET_LIST+=$(1)_TARGET
.PHONY: $(1)_TARGET
$(1)_TARGET:
	+$(MAKE) -C $(1)

VERSIONS_LIST+=$(1)_VERSIONS
.PHONY: $(1)_VERSIONS
$(1)_VERSIONS:
	+$(MAKE) -C $(1) all_versions

CLEAN_LIST+=$(1)_CLEAN
.PHONY: $(1)_CLEAN
$(1)_CLEAN:
	+$(MAKE) -C $(1) clean
endef


# Define the various targets via the Macro
$(foreach proj,$(PROJECTS),$(eval $(call TARGET,$(proj))))

all: $(TARGET_LIST)
	@echo Done building targets.

all_versions: $(VERSIONS_LIST)
	@echo Done building all versions.

clean: $(CLEAN_LIST)
	@echo Done cleaning targets.

run: all
	@echo Starting up python webserver.
	python $(HTTPD)

# uppercase aliases (for backward compatibility)
RUN: run

.PHONY: RUN run
