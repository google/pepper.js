#!/usr/bin/python

import sys

def main(argv):
  f = open(argv[0])
  out = []
  preamble = []
  def emit(line):
    out.append(line + '\n')
  for line in f:
    line = line.strip()
    if line and line[-1] != ';':
      preamble.append(line)
    elif preamble:
      preamble.append(line)
      emit(' '.join(preamble))
      preamble = []
    else:
      emit(line)
  assert not preamble

  f.close()

  f = open(argv[0], 'w')
  f.write(''.join(out))
  f.close()

  return 0

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
