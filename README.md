# Deplug #
Deplug is a JavaScript library that enables the compilation of PPAPI plugins into JavaScript using [Emscripten](https://github.com/kripken/emscripten).  This enables the deployment native code on the web both as a Native Client executable and as JavaScript.

## Getting Started ##
Warning: getting Deplug up and running currently requires a non-trivial amount work installing dependencies and configuring the system.  If you have Emscripten already working, however, it should be fairly simple.

### File Layout ###
* ./deplug/src => this git repo
* ./deplug/emscripten => [Emscripten](https://github.com/kripken/emscripten/wiki/Tutorial)
    * git clone https://github.com/kripken/emscripten.git
* ./nacl_sdk => the [NaCl SDK](https://developers.google.com/native-client/sdk/download)

### System Setup ###
Install Emscripten's dependencies:

* [Clang](http://llvm.org/releases/download.html)
* [node.js](http://nodejs.org/download/)
* [Python](http://www.python.org/download/)
* Java

Run ./deplug/emscripten/emcc to let Emscripten set itself up.  You may need to manually edit ~/.emscripten (after you've run emcc once) to point it to the correct paths.  For example, you may want Emscripten to use a hermetic version of Clang instead of the one installed on the system.  For a specific example, Emscripten doesn't appear to be entirely happy with Xcode's command line tools, so downloading prebuilt binaries from LLVM's website may save some difficulty, even if you're already set up with Xcode.

Development on Windows is currently not supported because Emscripten depends on Clang, and Clang does not officially support Windows.  In the future, the Windows will be supported via the PNaCl toolchain.  The PNaCl toolchain is essentially Clang, and it has already been ported to Windows.

Emscripten expects a "python2" binary to exist.  If it does not exist on your system, such as on OSX, you can create a symlink.
`sudo ln -s /usr/bin/python /usr/bin/python2`

### Building the Examples ###
    cd examples
    make V=1 TOOLCHAIN=emscripten CONFIG=Debug
    make V=1 TOOLCHAIN=emscripten CONFIG=Release
    make V=1 TOOLCHAIN=newlib CONFIG=Debug
    make V=1 TOOLCHAIN=newlib CONFIG=Release

### Running the Examples ###
    cd .. (out of the examples directory)
    python ./tools/httpd.py

Surf to [examples/examples.html](http://127.0.0.1:5103/examples/examples.html).
Note that this web server can be accessed remotely.

## Developing with Deplug ##

### PPAPI Interfaces ###

### Concurrency ###

### Required Compiler Flags ###
Building an example with `V=1` will show the flags being passed to Emscripten.

### Deploying Native Client ###

### Deploying JavaScript ###

## Getting Help ##
* [native-client-discuss](https://groups.google.com/forum/#!forum/native-client-discuss) for questions about Deplug and Native Client.
* [emscripten-discuss](https://groups.google.com/forum/#!forum/emscripten-discuss) for Emscripten-specific questions.
