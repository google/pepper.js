#!/usr/bin/python

"""
A simple script for finding missing symbols based on node.js error messages.
"""

import sys

import common

assert len(sys.argv) == 2, sys.argv

args = sys.argv[1].split(":")
assert len(args) == 3, args

fn = args[0]
ln = int(args[1])
cn = int(args[2])

with open(fn) as f:
    lineno = 0
    for line in f:
        lineno += 1
        if lineno == ln:
            # Take a line like this: __ZNK2pp3Var8AsStringEv($message, $1);
            # And produces this: _ZNK2pp3Var8AsStringEv
            print line.strip()
            print
            line = line[cn-1:].strip()
            assert line[0] == "_", line
            symbol = line[1:].split("(")[0]
            print symbol
            print
            print common.decodeSymbol(symbol)
            print

            break
