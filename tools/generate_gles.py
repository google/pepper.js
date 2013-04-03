#!/usr/bin/python

import os.path
import optparse
import sys

passthrough_list = """
activeTexture
blendColor
blendEquation
blendEquationSeparate
blendFunc
blendFuncSeparate
clearColor
clearDepth
clearStencil
colorMask
cullFace
depthFunc
depthMask
depthRange
disable
enable
frontFace
// TODO requires return values and type coersion.
//getParameter
getError
hint
isEnabled
lineWidth
pixelStorei
polygonOffset
sampleCoverage
stencilFunc
stencilFuncSeparate
stencilMask
stencilMaskSeparate
stencilOp
stencilOpSeparate

scissor
viewport

// TODO unwrap the uniform location.
//uniform1f
//uniform1i
//uniform2f
//uniform2i
//uniform3f
//uniform3i
//uniform4f
//uniform4i
vertexAttrib1f
vertexAttrib2f
vertexAttrib3f
vertexAttrib4f

clear
drawArrays
drawElements
finish
flush
"""

passthrough = set(passthrough_list.strip().split())

def toWebGLName(name):
  return name[0].lower() + name[1:]


def GenerateJsStubs(gen, interface, fo):
  prefix = interface.GetInterfaceString().split('_')[-1]

  funcs = [func for func in gen.original_functions if func.InPepperInterface(interface)]

  fo.write('\n\n')
  fo.write('    // %s\n' % interface.GetInterfaceString())
  fo.write('\n')

  for func in funcs:
    args = ['context_uid'] + [arg.name for arg in func.GetOriginalArgs()]

    fo.write('    // (%s) => %s\n' % (func.MakeTypedOriginalArgString(''), func.return_type))
    fo.write('    var %s_%s = function(%s) {\n' % (prefix, func.name, ', '.join(args)))
    fo.write('        var gl = resources.resolve(context_uid).ctx;\n');
    web_gl_name = toWebGLName(func.name)
    if web_gl_name in passthrough:
      return_stmt = 'return ' if func.return_type != 'void' else ''
      fo.write('        %sgl.%s(%s);\n' % (return_stmt, web_gl_name, ', '.join(args[1:])))
    else:
      fo.write('        throw "%s_%s not implemented";\n' % (prefix, func.name));
    fo.write('    };\n\n')

  fo.write('    registerInterface("%s;1.0", [\n' % interface.GetInterfaceString())
  for func in funcs:
    fo.write('        %s_%s,\n' % (prefix, func.name))
  fo.write('    ]);\n')


def main(argv):
  parser = optparse.OptionParser()
  parser.add_option('--chromium-src',
      help='base directory for Chromium.')

  options, args = parser.parse_args(args=argv)

  if options.chromium_src is None:
    parser.error('Must specify --chromium-src')
  if not os.path.isdir(options.chromium_src):
    parser.error('Chromium source directory does not exist: %s' % options.chromium_src)
  script = os.path.join('gpu', 'command_buffer', 'build_gles2_cmd_buffer.py')

  os.chdir(options.chromium_src)

  if not os.path.isfile(script):
    # TODO better error reporting.
    parser.error('Cannot find script: %s' % script)
  builder = {}
  execfile(script, builder)

  gen = builder['GLGenerator'](False)
  gen.ParseGLH("common/GLES2/gl2.h")

  dev = False

  fo = sys.stdout

  fo.write('(function() {\n')

  for interface in gen.pepper_interfaces:
    if interface.dev != dev:
        continue
    GenerateJsStubs(gen, interface, fo)
    print

  fo.write('})();\n');

  return 0

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
