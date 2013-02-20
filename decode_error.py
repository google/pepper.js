#!/usr/bin/python

"""
A simple script for finding missing symbols based on node.js error messages.
"""

import subprocess
import sys

assert len(sys.argv) == 4, sys.argv

fn = sys.argv[1]
ln = int(sys.argv[2])
cn = int(sys.argv[3])

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
            decoded = subprocess.check_output(['c++filt', symbol]).strip()
            print decoded
            print

            break
