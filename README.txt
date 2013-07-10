Warning getting Deplug up and running requires a non-trivial amount work
installing dependancies and configuring the system.  If you have Emscripten
already working, however, it should be fairly simple.


Assumed file layout:

./deplug/src => this git repo.

./deplug/emscripten => the built version of emscripten.
  git clone https://github.com/kripken/emscripten.git
  https://github.com/kripken/emscripten/wiki/Tutorial
  Dependancies:
    http://llvm.org/releases/download.html
    http://nodejs.org/download/
    (Also requires Java to run the Closure compiler.)

./nacl_sdk => the NaCl SDK
  https://developers.google.com/native-client/sdk/download


System setup notes:

Development on Windows is currently not supported because Emscripten depends on
Clang, and Clang does not officially support Windows.  In the future, the
Windows may be supported via the PNaCl toolchain.  The PNaCl toolchain is
essentially Clang, and it has already been ported to Windows.

Emscripten expects a "python2" binary to exist.  If it does not exist on your
system, such as on OSX, you can create a symlink.
sudo ln -s /usr/bin/python /usr/bin/python2

Emscripten doesn't appear to be entirely happy with Xcode's command line tools,
so downloading prebuilt binaries from LLVM's website may save some difficulty,
even if you're already set up with Xcode.

You may need to manually edit ~/.emscripten (after you've run emcc once) to
point it to the correct paths.  For example, you may want Emscripten to use a
hermetic version of Clang.


Building:

cd examples
make V=1 TOOLCHAIN=emscripten CONFIG=Debug -j32

Toolchains: emscripten, newlib
Configs: Debug, Release
-j controls the number of tasks executed in parallel.


Testing:

cd .. (out of the examples directory)
python ./tools/httpd.py
Surf to: 127.0.0.1:5103/examples/examples.html
Note that this web server can be accessed remotely.
