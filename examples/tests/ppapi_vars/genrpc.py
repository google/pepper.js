#!/usr/bin/python

import cStringIO
import json

class Interface(object):
  def __init__(self):
    self.methods = []

  def method(self, name):
    m = Method(name)
    m.uid = len(self.methods)
    self.methods.append(m)
    return m


class Method(object):
  def __init__(self, name):
    self.name = name
    self.uid = -1

class Block(object):
  def __init__(self, out, indent):
    self.out = out
    self.indent = indent

  def __enter__(self):
    self.old = self.out.indentText
    self.out.indentText += self.indent

  def __exit__(self, type, value, traceback):
    self.out.indentText = self.old


class CodePrinter(object):
  def __init__(self):
    self.out = cStringIO.StringIO()
    self.indentText = ""

  def string(self):
    return self.out.getvalue()

  def block(self, indent="  "):
    return Block(self, indent)

  def line(self, text):
    self.out.write(self.indentText)
    self.out.write(text)
    self.out.write("\n")

  def emptyLine(self):
    self.out.write("\n")

copyright = """
Copyright (c) 2013 Google Inc. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
"""

def getLines(text):
  lines = [line.rstrip() for line in text.split("\n")]
  while lines and lines[0] == "":
    lines.pop(0)
  while lines and lines[-1] == "":
    lines.pop()
  return lines

def cBlockComment(text, out):
  lines = getLines(text)
  out.line("/*")
  with out.block(" * "):
    for line in lines:
      out.line(line)
  out.line(" */")

def cppBlockComment(text, out):
  lines = getLines(text)
  with out.block("// "):
    for line in lines:
      out.line(line)

def generateCServer(interface):
  out = CodePrinter()
  cBlockComment(copyright, out)

  out.emptyLine()
  out.line("#include \"ppapi/c/pp_instance.h\"")
  out.line("#include \"ppapi/c/pp_var.h\"")

  out.emptyLine()
  for m in interface.methods:
    out.line("extern void %s(PP_Instance instance);" % m.name)

  #out.emptyLine()
  #out.line("void InitRPC(PPB_GetInterface get_browser_interface) {")
  #out.line("}")

  #out.emptyLine()
  #out.line("static const char* VarToString(struct PP_Var value) {")
  #with out.block():
  #  out.line("uint32_t len = 0;")
  #  out.line("char* s = NULL;")
  #  out.line("const char* raw = ppb_var->VarToUtf8(value, &len);")
  #  out.line("if (raw == NULL) return NULL;")
  #  out.line("s = malloc(len + 1);")
  #  out.line("memcpy(s, raw, len);")
  #  out.line("s[len] = 0;")
  #  # Note the "raw" pointer is owned by PPAPI.
  #  out.line("return s;")
  #out.line("}")

  out.emptyLine()
  out.line("static int GetInteger(struct PP_Var v) {")
  with out.block():
    out.line("if (v.type == PP_VARTYPE_DOUBLE) {")
    with out.block():
      out.line("return (int)v.value.as_double;")
    out.line("} else if (v.type == PP_VARTYPE_INT32) {")
    with out.block():
      out.line("return v.value.as_int;")
    out.line("} else {")
    with out.block():
      out.line("return -1;")
    out.line("}")
  out.line("}")

  out.emptyLine()
  out.line("void HandleMessage(PP_Instance instance, struct PP_Var message) {")
  with out.block():
    out.line("switch(GetInteger(message)) {")
    for m in interface.methods:
      out.line("case %d:" % m.uid)
      with out.block():
        out.line("%s(instance);" % m.name)
        out.line("break;")
    # TODO(ncbray): handle errors in default case?
    out.line("}")
  out.line("}")

  s = out.string()
  print s
  open("rpc.c", "w").write(s)

def generateJSClient(interface):
  out = CodePrinter()

  cppBlockComment(copyright, out)
  out.emptyLine()

  clsname = "RPC"

  out.line("var %s = function(embed) {" % clsname)
  with out.block():
    out.line("this.embed = embed;")
  out.line("};")

  for m in interface.methods:
    out.emptyLine()
    out.line("%s.prototype.%s = function() {" % (clsname, m.name))
    with out.block():
      out.line("this.embed.postMessage(%s);" % json.dumps(m.uid))
    out.line("};")

  s = out.string()
  print s
  open("rpc.js", "w").write(s)

if __name__ == "__main__":
  c = Interface()
  c.method("emptyArray")
  c.method("simpleArray")
  c.method("emptyDictionary")
  c.method("emptyArrayBuffer")
  c.method("simpleArrayBuffer")

  generateCServer(c)
  generateJSClient(c)
