# ppapi.js #
ppapi.js is a JavaScript library that enables the compilation of [Pepper](https://developers.google.com/native-client/pepperc/) plugins into JavaScript using [Emscripten](https://github.com/kripken/emscripten).  This allows the simultaneous deployment of native code on the web both as a [Native Client](http://gonacl.com) executable and as JavaScript. 

## Getting Started ##
Clone the repo.

Install the [NaCl SDK](https://developers.google.com/native-client/sdk/download).  Set the `NACL_SDK_ROOT` environment variable to be an absolute path that points to the desired pepper_* directory inside of the NaCl SDK.  pepper_30 and up are supported.  (The main dependency on pepper_30 is its toolchain, however, and not its header files and libraries.  It is possible to use an older version of Pepper by setting the `LLVM` environment variable to point to a toolchain other than the one contained in `NACL_SDK_ROOT`.)

Inside the ppapi.js repo, run:

    git submodule update --init

This will clone Emscripten.  Install Emscripten's dependencies on your system:

* [node.js 0.8 or above](http://nodejs.org/download/)
* [Python 2.7](http://www.python.org/download/)
* Java
* Optional: [Clang](http://llvm.org/releases/download.html)

On Ubuntu, you can run the following commands to install the dependencies:

    sudo apt-get install nodejs
    sudo apt-get install python27

Note, however, that Precise does not have a new enough version of node.js so you will need to build it from source.  If you get the following error while running Emscripten, your version of node.js is too old:

    TypeError: Object #<Object> has no method 'appendFileSync'

Emscripten expects a "python2" binary to exist.  If it does not exist on your system, such as on OSX, you can create a symlink.
`sudo ln -s /usr/bin/python /usr/bin/python2`

### Building the Examples ###
    cd examples
    make TOOLCHAIN=emscripten CONFIG=Debug
    make TOOLCHAIN=emscripten CONFIG=Release
    make TOOLCHAIN=newlib CONFIG=Debug
    make TOOLCHAIN=newlib CONFIG=Release
    make TOOLCHAIN=pnacl CONFIG=Debug
    make TOOLCHAIN=pnacl CONFIG=Release

When you run emscripten for the first time, it will build a few dependencies in the background and the compilation may appear to hang.  Emscripten will also create a create a configuration file named `~/.emscripten`.  You may need to edit this file if it fails to guess the correct paths, or you want to use a version other than the one it guessed.

There is a known issue where Emscripten will print the following warning when using the PNaCl toolchain:

    Warning: using an unexpected LLVM triple: armv7-none-linux-gnueabi, !== ,le32-unknown-nacl (are you using emcc for everything and not clang?)

It can be ignored and will be fixed in future releases.

### Clang ###
It should be noted that the build system is configured make Emscripten use the PNaCl toolchain instead of a standard version of Clang.  (PNaCl is essentially a modified version of Clang.)  This is done for two reasons.  First, it eliminates an install-time dependency.  Second, there is no standard binary distribution of Clang on Windows, but there is a Windows version of the PNaCl toolchain.  There appear to be a few issues with using the PNaCl toolchain, such as exception support, but they are being worked on.  If you want to use another version of Clang when compiling the examples, simply set the `LLVM` environment variable to point to it before invoking `make`.  Similarly, if you want to use the PNaCl toolchain when using Emscripten in other situations, either set the `LLVM` environment variable or edit `~/emscripten`.

If you want to use a version of Clang other than PNaCl, note that Emscripten doesn't appear to be entirely happy with Xcode's command line tools.  Downloading prebuilt binaries from LLVM's website or building them from source are the best options.

### Running the Examples ###
    cd .. (out of the examples directory)
    python ./tools/httpd.py

Surf to [examples/examples.html](http://127.0.0.1:5103/examples/examples.html).  Note that this web server can be accessed remotely, for better and for worse.

## Developing with ppapi.js ##
To create an application that can target both Native Client and Emscripten, it is necessary to work within the constraints of both platforms.  To do this, an application must be written to use the Pepper Plugin API and must _not_ use threads.

To be compatible with Native Client, it is necessary to use the [Pepper Plugin API](https://developers.google.com/native-client/pepperc/group___interfaces) to interact with the browser.  In the general case, Emscripten lets a developer define JavaScript functions that can be invoked synchronously from native code.  ppapi.js provides a set of JavaScript functions that implement Pepper and invoking any other JavaScript functions would break cross-toolchain compatibility.

To be compatible with Emscripten, it is necessary to structure the program so that it does not create additional threads and does not block the main thread.  In other words, the program must be written as if it were an event-driven JavaScript program.  The Pepper Plugin API imposes the same non-blocking requirement on its main thread of execution, so this constraint is equivalent to requiring that a Pepper plugin only runs on the main thread and does not create any additional threads.

Of course, both of these constraints can be worked around using the C preprocessor and conditional compilation.  For example, threading can be enabled on Native Client by guarding the relevant code with `#if defined(__native_client__) ... #endif`.  Emscripten-specific functionality can be conditioned on `defined(__EMSCRIPTEN__)`.  This approach is generally not recommended, but there are situations where the benefits outweigh the additional complexity - such as performance improvements from mutlithreading or calling directly to JavaScript rather than mediating through postMessage.

In addition to these two constraints, there are a few subtle differences between native code compiled with Native Client and compiled with Emscripten.  For example, dereferencing a null pointer (or accessing unmapped memory of any sort) will cause a segfault in Native Client whereas it will succeed in Emscripten and return junk data.  Developers should keep these platform differences in mind - similar to how differences between 32-bit and 64-bit architectures needs to be considered in other situations.

### Required Compiler Flags ###
Building an example with `V=1 TOOLCHAIN=emscripten` will show the flags being passed to Emscripten.  If you want to set up your own build system, there’s a few flags you must pass when linking in order for your application to use ppapi.js.  

    -s RESERVED_FUNCTION_POINTERS=325

ppapi.js creates function tables for each PPAPI interfaces at runtime.  Emscripten requires that space for each function pointer is reserved at link time.

    -s TOTAL_MEMORY=33554432

Emscripten defaults to a 16 MB address space, which may to be too small.  Tune the size for your particular application.

    -lppapi

The “ppapi” library contains boilerplate needed to bind the PPAPI plugin to JS.

    -s EXPORTED_FUNCTIONS="['_DoPostMessage', '_DoChangeView', '_DoChangeFocus', '_NativeCreateInstance', '_HandleInputEvent']"

These functions are called by ppapi.js, and they must be exported by your application.

TODO --js-library and --pre-js

TODO closure exports

## PPAPI Interfaces in ppapi.js ##

### Unsupported Interfaces ###
There are currently a few Pepper Interfaces not supported by ppapi.js.  For example, `PPB_MessageLoop` is not supported because it only makes sense when additional threads are created.  There are also a number of interfaces that simply haven’t been implemented, yet:

* `PPB_Gamepad`
* `PPB_MouseCursor`
* `PPB_TouchInputEvent`
* `PPB_VarArray`
* `PPB_VarDictionary`
* Networking-related interfaces
    * `PPB_HostResolver`
    * `PPB_NetAddress`
    * `PPB_NetworkProxy`
    * `PPB_TCPSocket`
    * `PPB_UDPSocket`
    * `PPB_WebSocket`

### Incomplete Support ###
ppapi.js was developed using test-driven development.  Features are only added when tests are available (either automatic or manual).  This means that even if an interface is supported, there may be missing features or subtle incompatibilities where test coverage is not available.  Lack of test coverage is the main reason ppapi.js has not been declared v1.0.

TODO figure out how to clearly explain how this situation impacts developers, or fix it.

### Implementation Errata ###
The Graphics2D and Graphics3D interfaces will automatically swap buffers every frame, even if Flush or SwapBuffers is not called. This behavior should not be noticeable for most applications. Explicit swapping could be emulated by creating an offscreen buffer, but this would cost time and memory.

Graphics3D may not strictly honor `PP_GRAPHICS3DATTRIB_*` parameters but best effort will be made to do something reasonable.  [WebGL](https://www.khronos.org/registry/webgl/specs/1.0/) provides less control than PPAPI, and ppapi.js is implemented on top of WebGL.  For example, if a 24-bit depth buffer is requested there will be a depth buffer but WebGL only makes guarantees that depth buffers are at least 16 bits.

Using BGRA image formats will result in a silent performance penalty. In general, web APIs tend to be strongly opinionated that premultiplied RGBA is the image format that should be used. Any other format must be manually converted into premultiplied RGBA.

The Audio API only supports one sample rate - whatever the underlying Web Audio API uses, which is whatever the OS defaults to, which tends to be either 44.1k or 48k. 48k appears to be a little more common.  This means that an app expecting a particular sample rate may not be able to get it, and this can cause serious difficulties.  In the future, resampling could be performed as a polyfill, but this would be slow.

URLLoader intentionally deviates from the native implementation's behavior when it is at odds with XMLHttpRequest. For example, ppapi.js does not identify CORS failures as `PP_ERROR_NOACCESS`, instead it returns `PP_ERROR_FAILED`.

URLLoader does not stream - the data appears all at once. This is a consequence of doing an XHR with requestType "arraybuffer", it does not appear to give partial results.

If multiple mouse buttons are held, ppapi.js will list all of them as event modifiers. PPAPI will only list one button - the one with the lowest enum value. There is a known but where ppapi.js will not update the modifier state if a button is pressed or released outside of ppapi.js's canvas.

### Platform Errata ###
`PPB_Graphics3D` does not work on Internet Explorer 10 or before because WebGL is not supported.  WebGL is supported on Safari, but it must be [manually enabled](https://discussions.apple.com/thread/3300585).

`PPB_MouseLock` and `PPB_Fullscreen` are only supported in Chrome and Firefox.  The behavior of these interfaces varies somewhat between the two browsers, however.  Safari supports fullscreen, but does not support mouse lock.

The file interfaces are currently supported only by Chrome. (Creation and last access time are not supported, even on Chrome.) A polyfill for Firefox and IE is included in ppapi.js, but it has a few known bugs - such as not being able to resize existing files. Another issue is that the Closure compiler will rename fields in persistent data structures, resulting in data incompatibility/loss between Debug and Release versions, and possibly even between different Release versions.

Chrome will smoothly scale the image composited into the page when using ppapi.js, all other browsers will do nearest-neighbor scaling.  Nexes and pexes will also do nearest-neighbor scaling.  This means low res or pixel style graphics will be slightly blurred on Chrome with ppapi.js, unless the back buffer is the same size as the view port and the scaling factor for high DPI displays is accounted for.

Input events are a little fiddly due to inconsistencies between browsers. For example, the delta for scroll wheel events is scaled differently in different browsers. ppapi.js attempts to normalize this, but in general, cross-platform inconsistencies should be expected in the input event interface.

Mobile browsers have not been tested.

TODO interface support dashboard for programmatically showing interface support.

## Deployment ##
ppapi.js lets a single Pepper plugin be deployed as both a Native Client executable and as JavaScript.  Choosing a single technology and sticking with it would make life simpler, but there are advantages and disadvantages to each technology.  Deploying different technologies in different circumstances let an application play to the strengths of each.

Native Client generally provides better performance than JavaScript, particularly when threading is leveraged.  On the downside, currently Native Client executables are only supported by Chrome.  JavaScript has much more pervasive browser support.  It should be noted that although JavaScript “runs everywhere,” performance can vary widely between browsers, sometimes to the point of making an application unusable.  Because of this, It is highly suggested that applications be designed to scale across differing amounts processing power, if possible.

### Portable Native Client ###
In addition to only running on Chrome, Native Client is further restricted to only run as a Chrome (web app)[http://developer.chrome.com/extensions/apps.html].  Native Client executables contain architecture-specific code, which makes them inappropriate for running on the open web.  There is, however, an architecture neutral version of Native Client called Portable Native Client.  Portable Native Client executables contain platform-neutral bitcode, making it better suited for the open web.  Starting in Chrome 30, PNaCl executables can be loaded in arbitrary web pages.  Initial load times are longer than because bitcode must be translated into architecture-specific code before it is executed.  For applications running on the open web, PNaCl is required, but when deploying as a Chrome App, it may be advantageous to use NaCl.

## Getting Help ##
* [native-client-discuss](https://groups.google.com/forum/#!forum/native-client-discuss) for questions about ppapi.js and Native Client.
* [emscripten-discuss](https://groups.google.com/forum/#!forum/emscripten-discuss) for Emscripten-specific questions.
