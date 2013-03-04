#!/usr/bin/python

import re
import sys

import common

stub_finder = re.compile('//\s*stub\s+for\s+(\S+)')

def main():
  symbols = []

  f = open(sys.argv[1])
  for line in f:
    m = stub_finder.search(line)
    if not m: continue
    symbol = m.group(1)
    decoded = common.decodeSymbol(symbol[1:])
    symbols.append(decoded)

  symbols.sort()
  for decoded in symbols:
    print "Missing: %s" % decoded

main()
