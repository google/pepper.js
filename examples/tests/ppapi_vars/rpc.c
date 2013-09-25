/*
 * Copyright (c) 2013 Google Inc. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_var.h"

extern void emptyDictionary(PP_Instance instance);

static int GetInteger(struct PP_Var v) {
  if (v.type == PP_VARTYPE_DOUBLE) {
    return (int)v.value.as_double;
  } else if (v.type == PP_VARTYPE_INT32) {
    return v.value.as_int;
  } else {
    return -1;
  }
}

void HandleMessage(PP_Instance instance, struct PP_Var message) {
  switch(GetInteger(message)) {
  case 0:
    emptyDictionary(instance);
    break;
  }
}
