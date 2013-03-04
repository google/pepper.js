import subprocess

def decodeSymbol(symbol):
  return subprocess.check_output(['c++filt', symbol]).strip()
