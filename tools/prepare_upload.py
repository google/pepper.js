#!/usr/bin/python

import os.path
import re
import posixpath
import subprocess

# Unfortunately the build products are scattered throughout the examples
# directory, so filter by type
filetype = re.compile("\.(js|html|css|nmf|nexe|pexe|jpg|png|raw|frag|vert|js\.map)$")

os.chdir("examples")

def build(toolchain, mode):
  subprocess.check_call(["make", "CONFIG=" + mode, "TOOLCHAIN=" + toolchain])

# Make sure everything is built.
for toolchain in ["newlib", "pnacl", "emscripten"]:
  for mode in ["Debug", "Release"]:
    build(toolchain, mode)

def upload(fn):
  cmd = ["gsutil", "cp", "-z", "html,js,css,nmf,nexe,pexe,map", "-a", "public-read", fn,  "gs://" + posixpath.normpath(posixpath.join("gonacl/pepper.js", fn))]
  print " ".join(cmd)
  subprocess.check_call(cmd)

# Upload.
for path, dirs, files in os.walk("."):
  path = os.path.relpath(path, ".")
  for f in files:
    if not filetype.search(f):
      continue
    if f.endswith(".nexe") and "unstripped" in f:
      continue
    full = os.path.join(path, f)
    print full
    upload(full)
