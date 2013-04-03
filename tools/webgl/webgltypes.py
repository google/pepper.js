#!/usr/bin/python

import collections
import re
import sys

func_spliter = re.compile(r'^(.*\S)\s+\(?\*?([^()]+)\)?\(([^(]*)\);$')

arg_spliter = re.compile(r',\s*')

Type = collections.namedtuple('Type', 'text base const pointer')
Arg = collections.namedtuple('Arg', 'name type')
Signature = collections.namedtuple('Signature', 'name args return_type')

def parse_type(t):
  text = t
  const = False
  if t.startswith('const '):
    const = True
    t = t[6:]

  pointer = 0
  while 1:
    if t.endswith('[]'):
      t = t[:-2]
    elif t.endswith('*'):
      t = t[:-1]
    else:
      break
    pointer += 1
  
  assert ' ' not in t, t
  return Type(text, t, const, pointer)


def parse_arg(arg):
  atype, aname = arg.strip().rsplit(' ', 1)
  return Arg(aname, parse_type(atype))


def parse_interface(filename):
  sigs = []
  f = open(filename)
  for line in f:
    line = line.strip()
    if not line: continue
    return_type, name, args = func_spliter.match(line).groups()
    argnodes = []
    if args:
      args = arg_spliter.split(args)
      for arg in args:
        argnodes.append(parse_arg(arg))
    node = Signature(name, argnodes, parse_type(return_type))
    sigs.append(node)
  return sigs

def first_to_lower(s):
  return s[0].lower() + s[1:]


name_remap = {
  "ClearDepthf": "clearDepth",
  "DepthRangef": "depthRange",
}

handwritten = {
  "BufferData": """
var _context = resources.resolve(context);
var _data = HEAP8.subarray(data, data + size);
_context.ctx.bufferData(target, _data, usage);
""",

  "GenBuffers": """
var _context = resources.resolve(context);
for (var i = 0; i < n; i++) {
    setValue(buffers + i * 4, resources.register("buffer", {native: _context.ctx.createBuffer()}), 'i32');
}
""",

  "GenTextures": """
var _context = resources.resolve(context);
for (var i = 0; i < n; i++) {
    setValue(textures + i * 4, resources.register("texture", {native: _context.ctx.createTexture()}), 'i32');
}
""",

  "ShaderSource": """
var _context = resources.resolve(context);
var _shader = resources.resolve(shader).native;
var chunks = [];
for (var i = 0; i < count; i++) {
    var str_ptr = getValue(str + i * 4, 'i32');
    var l = 0;
    if (length != 0) {
        l = getValue(length + i * 4, 'i32');
    }
    if (l <= 0) {
        chunks.push(Pointer_stringify(str_ptr));
    } else {
        chunks.push(Pointer_stringify(str_ptr, l));
    }
}
_context.ctx.shaderSource(_shader, chunks.join(""));
""",

  "TexImage2D": """
var _context = resources.resolve(context);
// TODO handle other pixel formats.
if (type != 0x1401) throw "Pixel format not supported.";
var _pixels = HEAPU8.subarray(pixels, pixels + width * height * 4);
_context.ctx.texImage2D(target, level, internalformat, width, height, border, format, type, _pixels);
""",

  "UniformMatrix4fv": """
var _context = resources.resolve(context);
var _location = resources.resolve(location).native;
var _value = HEAPF32.subarray((value>>2), (value>>2) + 16 * count);
_context.ctx.uniformMatrix4fv(_location, transpose, _value);
""",
}


def possibleForPPAPI(sig, webgl_lut):
  webgl_name = name_remap.get(sig.name, first_to_lower(sig.name))
  possible = webgl_lut[webgl_name]
  return possible

class CannotWrap(Exception):
  pass

simple_types = set(["GLfloat", "GLint", "GLuint", "GLsizei", "GLenum", "GLclampf", "GLbitfield", "GLboolean"])

def simpleType(t):
  return t.base in simple_types and not t.pointer

def simpleTypeMatch(t1, t2):
  return simpleType(t1) and simpleType(t2) and t1.base == t2.base

def isBufferOffset(t1, t2):
  return t1.pointer == 1 and t1.base == "void" and t2.pointer == 0 and t2.base == "GLintptr"

def isBufferType(t1, t2):
  return t1.text == 'GLuint' and t2.text == 'WebGLBuffer'

def isProgramType(t1, t2):
  return t1.text == 'GLuint' and t2.text == 'WebGLProgram'

def isShaderType(t1, t2):
  return t1.text == 'GLuint' and t2.text == 'WebGLShader'

def isTextureType(t1, t2):
  return t1.text == 'GLuint' and t2.text == 'WebGLTexture'

def isUniformLocationType(t1, t2):
  return t1.text == 'GLint' and t2.text == 'WebGLUniformLocation'

def isFramebufferType(t1, t2):
  return t1.text == 'GLuint' and t2.text == 'WebGLFramebuffer'

def isRenderbufferType(t1, t2):
  return t1.text == 'GLuint' and t2.text == 'WebGLRenderbuffer'

def isNullTerminatedStringType(t1, t2):
  return t1.text == 'const char*' and t2.text == 'DOMString'


def isVertexAttrib(ppapi_sig):
  return (ppapi_sig.name.endswith("iv") or ppapi_sig.name.endswith("fv")) and ppapi_sig.return_type.text == "void" and len(ppapi_sig.args) == 3

def signatureComment(ppapi_sig, webgl_sig):
  ppapi_text = "(%s) => %s" % (", ".join([arg.type.text for arg in ppapi_sig.args[1:]]), ppapi_sig.return_type.text)
  webgl_text = "(%s) => %s" % (", ".join([arg.type.text for arg in webgl_sig.args]), webgl_sig.return_type.text)
  if ppapi_text == webgl_text:
    return ["// " + ppapi_text]
  else:
    return ["// ppapi " + ppapi_text, "// webgl " + webgl_text]


def block(lines):
  return ["    " + line for line in lines]

def ppapiWrapperFunc(ppapi_sig):
  js_name = "OpenGLES2_" + ppapi_sig.name
  args = ', '.join([arg.name for arg in ppapi_sig.args])
  return "var %s = function(%s) {" % (js_name, args)


def unwrapDataPointer(ppapi_sig):
  lines = [ppapiWrapperFunc(ppapi_sig)]
  target_name = "OpenGLES2_" + ppapi_sig.name[:-1]
  tc = ppapi_sig.name[-2]
  if tc == "i":
    heap_access = "HEAP32[(%s>>2)+%d]"
  elif tc == "f":
    heap_access = "HEAPF32[(%s>>2)+%d]"
  else:
    assert False, ppapi_sig
  arg_count = int(ppapi_sig.name[-3])

  args = [arg.name for arg in ppapi_sig.args[:2]]
  for i in range(arg_count):
    args.append(heap_access % (ppapi_sig.args[2].name, i))

  body = []
  body.append("%s(%s);" % (target_name, ", ".join(args)))

  lines.extend(block(body))
  lines.append("}")
  lines.append("")
  return lines

def unwrapArg(template, arg_name):
  temp = "_" + arg_name
  return "var %s = %s;" % (temp, template % arg_name), temp

def makeHandwritten(ppapi_sig, webgl_lut):
  lines = []
  lines.append(ppapiWrapperFunc(ppapi_sig))

  stmts = [stmt.rstrip() for stmt in handwritten[ppapi_sig.name].split("\n")]
  while stmts[0] == '': stmts.pop(0)
  while stmts[-1] == '': stmts.pop()

  lines.extend(block(stmts))
  lines.append("}")

  return lines  

def generateWrapper(ppapi_sig, webgl_lut):
  if ppapi_sig.name in handwritten:
    lines = makeHandwritten(ppapi_sig, webgl_lut)
    print "\n".join(block(lines))
    #assert False
    return
  possible = possibleForPPAPI(ppapi_sig, webgl_lut)
  if len(possible) == 0:
    raise CannotWrap("No corresponding implementation")
  elif len(possible) > 1:
    if isVertexAttrib(ppapi_sig):
      lines = unwrapDataPointer(ppapi_sig)
      print "\n".join(block(lines))
      return
    raise CannotWrap("Cannot deal with overloads")

  webgl_sig = possible[0]

  context = ppapi_sig.args[0]
  ppapi_args = ppapi_sig.args[1:]
  webgl_args = webgl_sig.args[0:]

  ppapi_index = 0
  webgl_index = 0

  lines = []
  input_names = []

  lines.extend(signatureComment(ppapi_sig, webgl_sig))
  lines.append(ppapiWrapperFunc(ppapi_sig))

  body = []

  stmt, context = unwrapArg('resources.resolve(%s)', context.name)
  body.append(stmt)

  while ppapi_args and webgl_args:
    ppapi_arg = ppapi_args.pop(0)
    webgl_arg = webgl_args.pop(0)
    if simpleTypeMatch(ppapi_arg.type, webgl_arg.type) or isBufferOffset(ppapi_arg.type, webgl_arg.type):
      input_names.append(ppapi_arg.name)
    elif isBufferType(ppapi_arg.type, webgl_arg.type):
      stmt, temp = unwrapArg("resources.resolve(%s).native", ppapi_arg.name)
      body.append(stmt)
      input_names.append(temp)
    elif isProgramType(ppapi_arg.type, webgl_arg.type):
      stmt, temp = unwrapArg("resources.resolve(%s).native", ppapi_arg.name)
      body.append(stmt)
      input_names.append(temp)
    elif isShaderType(ppapi_arg.type, webgl_arg.type):
      stmt, temp = unwrapArg("resources.resolve(%s).native", ppapi_arg.name)
      body.append(stmt)
      input_names.append(temp)
    elif isTextureType(ppapi_arg.type, webgl_arg.type):
      stmt, temp = unwrapArg("resources.resolve(%s).native", ppapi_arg.name)
      body.append(stmt)
      input_names.append(temp)
    elif isUniformLocationType(ppapi_arg.type, webgl_arg.type):
      stmt, temp = unwrapArg("resources.resolve(%s).native", ppapi_arg.name)
      body.append(stmt)
      input_names.append(temp)
    elif isRenderbufferType(ppapi_arg.type, webgl_arg.type):
      stmt, temp = unwrapArg("coerceRenderbuffer(%s)", ppapi_arg.name)
      body.append(stmt)
      input_names.append(temp)
    elif isFramebufferType(ppapi_arg.type, webgl_arg.type):
      stmt, temp = unwrapArg("coerceFramebuffer(%s)", ppapi_arg.name)
      body.append(stmt)
      input_names.append(temp)
    elif isNullTerminatedStringType(ppapi_arg.type, webgl_arg.type):
      stmt, temp = unwrapArg("Pointer_stringify(%s)", ppapi_arg.name)
      body.append(stmt)
      input_names.append(temp)
    else:
      if ppapi_arg.type.text == webgl_arg.type.text:
        raise Exception(ppapi_arg.type.text)
      raise CannotWrap("Cannot coerce argument type <%s> to <%s>" % (ppapi_arg.type.text, webgl_arg.type.text))

  if ppapi_args or webgl_args:
    raise CannotWrap("Arg count mismatch")

  expr = "%s.ctx.%s(%s)" % (context, webgl_sig.name, ", ".join(input_names))

  returns_value = True
  if ppapi_sig.return_type.text == 'void':
    assert webgl_sig.return_type.text == 'void'
    returns_value = False
  elif isShaderType(ppapi_sig.return_type, webgl_sig.return_type):
    expr = "resources.register(\"gles_shader\", {native: %s})" % expr
  elif isProgramType(ppapi_sig.return_type, webgl_sig.return_type):
    expr = "resources.register(\"gles_program\", {native: %s})" % expr
  elif isUniformLocationType(ppapi_sig.return_type, webgl_sig.return_type):
    expr = "resources.register(\"gles_uniform_location\", {native: %s})" % expr
  else:
    if not simpleTypeMatch(ppapi_sig.return_type, webgl_sig.return_type):
      raise CannotWrap("Cannot coerce return type <%s> to <%s>" % (webgl_sig.return_type.text, ppapi_sig.return_type.text))

  if returns_value:
    expr = "return " + expr
  body.append(expr + ";")

  lines.extend(block(body))
  lines.append("}")

  print "\n".join(block(lines))
  print


def generateStub(ppapi_sig):
      js_name = "OpenGLES2_" + ppapi_sig.name
      args = ', '.join([arg.name for arg in ppapi_sig.args])
      return """    var %s = function(%s) {
        throw "%s not implemented";
    }
""" % (js_name, args, js_name)


def main(argv):
  ppapi = parse_interface('ppapi_interface.txt')
  webgl = parse_interface('webgl_interface.txt')
  webgl_lut = collections.defaultdict(list)
  for sig in webgl:
    webgl_lut[sig.name].append(sig)


  print "(function() {"

  problems = []

  generated = 0
  failed = 0

  funcs = []

  for sig in ppapi:
    try:
      generateWrapper(sig, webgl_lut)
      generated += 1
    except CannotWrap, e:
      problems.append("%s: %s" % (sig.name, e.message))
      print generateStub(sig)
      failed += 1
    funcs.append("OpenGLES2_" + sig.name + ",")

  lines = []
  lines.append("registerInterface(\"PPB_OpenGLES2;1.0\", [")
  lines.extend(block(funcs))
  lines.append("]);");
  print "\n".join(block(lines))
  print "})();"

  for problem in problems:
    print "// " + problem
  print "// %d:%d" % (generated, failed)

  return 0

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
