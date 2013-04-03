#!/usr/bin/python

import re
import sys

initial_split = re.compile(r'(\S*)\s+\(\*([A-Za-z0-9_]*)\)\(([^)]*)\)')

funcs = []

def handleDecl(decl, prefix, fo):
  m = initial_split.search(decl)
  ret_type, name, args = m.groups()
  args = [arg.strip() for arg in args.split(',')]
  names = [arg.split()[-1].strip('[]') for arg in args]

  full_name = '%s_%s' % (prefix, name)

  funcs.append(full_name)

  fo.write('\n')
  fo.write('    var %s = function(%s) {\n' % (full_name, ', '.join(names)))
  fo.write('        throw "%s not implemented";\n' % full_name)
  fo.write('    };\n')


def main(argv):
  assert len(argv) == 1
  fi = open(argv[0])

  fo = sys.stdout

  fo.write('(function() {\n')


  name = fi.readline().strip()

  prefix = name.split(';')[0].split('_')[1]

  for line in fi:
    line = line.strip()
    decl = handleDecl(line, prefix, fo)

  fo.write('\n')
  fo.write('    registerInterface("%s", [\n' % name)
  for name in funcs:
    fo.write('        %s,\n' % name)
  fo.write('    ]);\n')

  fo.write('})();\n')

  return 0

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
