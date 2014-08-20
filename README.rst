=========
pepper.js
=========

pepper.js is a JavaScript library that enables the compilation of native Pepper_
applications into JavaScript using Emscripten_. This allows the simultaneous
deployment of native code on the web both as a `Portable Native Client`_ (PNaCl)
executable and as JavaScript. Native Pepper applications can now be run in
Chrome, Firefox, Internet Explorer, Safari, and more.

.. _Pepper: https://developers.google.com/native-client/pepperc/
.. _Emscripten: https://github.com/kripken/emscripten
.. _`Portable Native Client`: http://gonacl.com

--------------
Project Status
--------------

pepper.js is incomplete and under development.  Expect that you'll need to get
your hands dirty.  Bug reports, feature requests, and patches are welcome.

Note: for more up-to-date documentation, check out the wiki_.

.. _wiki: https://github.com/google/pepper.js/wiki

-------------
Upgrade Notes
-------------

Older version of pepper.js included Emscripten to simplify the install process.
This approach resulted in some rather unfortunate versioning issues, so it has
been discontinued.  To use pepper.js, you must now install Emscripten yourself.
This will be covered in the "Getting Started" section.  If you have previously
used pepper.js, you will need to upgrade:

* Install the `Emscripten SDK`_.
* Manually delete the "emscripten" directory from this repo.
* Make sure either emcc is in your search path (prefered), or the ``EMSCRIPTEN``
  environment variable is set.

.. _`Emscripten SDK`: http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html

---------------
Getting Started
---------------

Clone this repo using git_.

Install the `NaCl SDK`_.  Set the ``NACL_SDK_ROOT`` environment variable to be
an *absolute* path that points to the desired ``pepper_*`` directory inside of
the NaCl SDK.  For example, to temporarily set the environment variable:

::

    set NACL_SDK_ROOT=c:\path\to\nacl_sdk\pepper_31  (Windows)
    export NACL_SDK_ROOT=/path/to/nacl_sdk/pepper_31  (Almost everything else)

``pepper_31`` and up are supported.  (The main dependency on
``pepper_31`` is its toolchain, however, and not its header files and libraries.
It is possible to use an older version of Pepper by setting the ``LLVM``
environment variable to point to a toolchain other than the one contained in
``NACL_SDK_ROOT``.)


Install the `Emscripten SDK`_.

Note that Ubuntu Precise does not have a new enough version of node.js. If you
accidently call the system version of node.js, you get the following error:

::

    TypeError: Object #<Object> has no method 'appendFileSync'

Emscripten expects a ``python2`` binary to exist.  If it does not exist on your
system, such as on OSX, you can create a symlink.
``sudo ln -s /usr/bin/python /usr/bin/python2``

.. _git: http://git-scm.com/downloads
.. _`NaCl SDK`: https://developers.google.com/native-client/sdk/download
.. _`Emscripten SDK`: http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html

Building the Examples
---------------------

::

    cd examples
    make TOOLCHAIN=emscripten CONFIG=Debug
    make TOOLCHAIN=emscripten CONFIG=Release
    make TOOLCHAIN=pnacl CONFIG=Debug
    make TOOLCHAIN=pnacl CONFIG=Release

When you run Emscripten for the first time, it will build a few dependencies in
the background and the compilation may appear to hang.  Emscripten will also
create a create a configuration file named ``~/.emscripten``.  You may need to
edit this file if it fails to guess the correct paths, or you want to use a
version other than the one it guessed.  If you install the Emscripten SDK, this
should not be a problem.

Running the Examples
--------------------

::

    cd .. (out of the examples directory)
    python ./tools/httpd.py

Surf to http://127.0.0.1:5103/examples/examples.html.  Note that this web server
can be accessed remotely, for better and for worse.

Note that to run the NaCl examples you'll need to pass ``--enable-nacl`` to
Chrome on the command line or enable it in ``about:flags``.

-------------------------
Developing with pepper.js
-------------------------

To create an application that can target both Native Client and Emscripten, it
is necessary to work within the constraints of both platforms.  To do this, an
application must be written to use the Pepper Plugin API, must *not* use
threads, and must *not* rely on memory mapping or any sort of memory page
protection.

To be compatible with Native Client, it is necessary to use the Pepper_ Plugin
API to interact with the browser.  In the general case, Emscripten lets a
developer define JavaScript functions that can be invoked synchronously from
native code.  pepper.js provides a set of JavaScript functions that implement
Pepper and invoking any other JavaScript functions would break cross-toolchain
compatibility.

To be compatible with Emscripten, it is necessary to structure the program so
that it does not create additional threads and does not block the main thread.
In other words, the program must be written as if it were an event-driven
JavaScript program.  The Pepper Plugin API imposes the same non-blocking
requirement on its main thread of execution, so this constraint is equivalent to
requiring that a Pepper plugin only runs on the main thread and does not create
any additional threads.

Emscripten also exposes a simplified version of the traditional native memory
model: memory is a linear array.  This means that page protections do not exist,
memory accesses never fault, and mmap is not supported.  The Pepper Plugin API
`implicitly uses mmap`_ in a few of its APIs, and pepper.js emulates mmaping by
silently copying on use any memory that may have been modified.  This approach
has obvious performance implications, but for the moment it provides the best
emulation of Pepper’s semantics.

.. _`implicitly uses mmap`: https://developers.google.com/native-client/pepperc/struct_p_p_b___image_data__1__0

Note: not having page protections results in a subtle "gotcha" when porting to
Emscripten.  Dereferencing a null pointer (or accessing unmapped memory of any
sort) will cause a segfault in Native Client (and pretty much any other native
platform) whereas it will succeed in Emscripten and return junk data.  According
to the C spec, dereferencing a null pointer results in `undefined behavior`_, so
this is theoretically "working as intended".  In practice, however, existing
code may rely on null pointer dereferences causing memory faults to implicitly
assert a pointer is not null.  This is a subtle portability issue for Emscripten
and generally a `bad idea`_, even when not targeting Emscripten.

.. _`undefined behavior`: http://blog.llvm.org/2011/05/what-every-c-programmer-should-know.html
.. _`bad idea`: http://codearcana.com/posts/2013/04/23/exploiting-a-go-binary.html

Of course, all of these constraints can be worked around using the C
preprocessor and conditional compilation.  For example, threading can be enabled
on Native Client by guarding the relevant code with ``#if
defined(__native_client__) ... #endif``.  Emscripten-specific functionality can
be conditioned on ``defined(__EMSCRIPTEN__)``.  This approach is generally not
recommended, but there are situations where the benefits outweigh the additional
complexity - such as performance improvements from multithreading or calling
directly to JavaScript rather than mediating through postMessage.

C++ Exceptions
--------------

The use of C++ exceptions is currently discouraged for two reasons.  First,
Emscripten disables exception handling by default for ``-O1`` and higher. This
can be overridden by passing ``-s DISABLE_EXCEPTION_CATCHING=0`` to Emscripten,
but doing so *may* or may not result in a noticeable performance penalty.
Additional code will be generated at every call site an exception could
propagate through.  Second, exceptions are `currently not supported`_ by PNaCl.

.. _`currently not supported`: https://code.google.com/p/nativeclient/issues/detail?id=2798

----------
Deployment
----------

pepper.js lets a single Pepper plugin be deployed as both a Native Client
executable and as JavaScript.  Choosing a single technology and sticking with it
would make life simpler, but there are advantages and disadvantages to each
technology.  Deploying different technologies in different circumstances let an
application play to the strengths of each.

Native Client generally provides better performance than JavaScript,
particularly when threading is leveraged.  On the downside, Native Client
executables are currently only supported by Chrome.  JavaScript has much more
pervasive browser support.  It should be noted that although JavaScript "runs
everywhere," performance can vary widely between browsers, even on the same
hardware.  Web users also have a wide spectrum of CPU and GPU power.  If
possible, design your applications to scale across differing amounts of
processing power, no matter which technology is being used.

In terms of file size, it appears that Native Client and Emscripten produce
executables of roughly the same size, once they are stripped/minimized and
gzipped.  They are different versions of the same program, so it is unsurprising
their compressed sizes are similar.

Portable Native Client
----------------------

In addition to only running on Chrome, the original version of Native Client is
further restricted to only run as a `Chrome Web App`_.  Native Client
executables contain architecture-specific code, which makes them inappropriate
for running on the open web.  There is, however, an architecture neutral version
of Native Client called Portable Native Client.  Portable Native Client
executables contain platform-neutral bitcode, making it better suited for the
open web.  Starting in Chrome 31, PNaCl executables can be loaded in arbitrary
web pages.  For applications running on the open web, PNaCl is required, but
when deploying as a Chrome App, it may be advantageous to use NaCl.

.. _`Chrome Web App`: http://developer.chrome.com/extensions/apps.html

--------------------------
Build System Configuration
--------------------------

Note: configuring the build system to use pepper.js is currently a little
complicated.  The instructions will likely change in future versions.  Expect
that you may need to update your build when pulling a new version of pepper.js.

Building an example with ``V=1 TOOLCHAIN=emscripten`` will show the flags being
passed to Emscripten.  If you want to set up your *own* build system, there's a
few flags you must pass to the linker to use pepper.js.  Here's a flag-by-flag
breakdown of what's going on when the examples are built.

::

    -s RESERVED_FUNCTION_POINTERS=325

pepper.js creates function tables for each PPAPI interfaces at runtime.
Emscripten requires that space for each function pointer is reserved at link
time.

::

    -s TOTAL_MEMORY=33554432

Emscripten defaults to a 16 MB address space, which may to be too small.  Tune
the size for your particular application.

::

    -lppapi

The "ppapi" library contains boilerplate needed to bind the PPAPI plugin to JS.

::

    -s EXPORTED_FUNCTIONS="['_DoPostMessage', '_DoChangeView', '_DoChangeFocus', '_NativeCreateInstance', '_HandleInputEvent']"

These functions are called by pepper.js, and they must be exported by your
application.

To make pepper.js work Emscripten needs to include a number of files using the
``--pre-js`` flag.  In all cases, ``ppapi_preamble.js`` must be included.
Depending on what interfaces the program being compiled needs, the corresponding
files in the ``wrappers/`` directory must be included.  If you are using the
File IO API, you will also need to include ``third_party/idb.filesystem.js``.
This situation will hopefully be changed in the future to minimize the number of
command line flags required.

::

    --closure 1

Emscripten has a built-in option to use the `Closure Compiler` to minimize the
JavaScript it generates.  This option should only be used for release builds
because minification obfuscates the generated code, similar to optimization
passes in C compilers. The minimization process renames variables and methods.
To maintain correctness, the Closure Compiler needs to avoid renaming variables
and methods that are built in to the browser.  If it renames built-in names, the
resulting program breaks.  pepper.js uses a number of relatively new APIs that
Closure does not know about, yet.  Closure will mangle these names unless it is
explicitly told to preserve them.  To prevent these APIs from being mangled,
they can be declared "extern" in a JavaScript file and passed to Closure.
Emcsripten calls Closure internally, and extern declarations must be tunneled to
Closure through an environment variable rather than being passed on the command
line.

.. _`Closure Compiler`: https://developers.google.com/closure/compiler/

::

    EMCC_CLOSURE_ARGS=--externs $(PEPPERJS_SRC_ROOT)/externs.js --externs $(PEPPERJS_SRC_ROOT)/third_party/w3c_audio.js

-----------------------------
PPAPI Interfaces in pepper.js
-----------------------------

Unsupported Interfaces
----------------------

There are currently a few Pepper Interfaces not supported by pepper.js.  For
example, ``PPB_MessageLoop`` is not supported because it only makes sense when
additional threads are created.  There are also a number of interfaces that
simply haven’t been implemented, yet:

* ``PPB_Gamepad``
* ``PPB_MouseCursor``
* ``PPB_TouchInputEvent``
* Networking-related interfaces
    * ``PPB_HostResolver``
    * ``PPB_NetAddress``
    * ``PPB_NetworkProxy``
    * ``PPB_TCPSocket``
    * ``PPB_UDPSocket``
    * ``PPB_WebSocket``

Incomplete Support
------------------

pepper.js was developed using test-driven development.  Features are only added
when tests are available (either automatic or manual).  This means that even if
an interface is supported, there may be missing features or subtle
incompatibilities where test coverage is not available.  Lack of test coverage
will be the main difficulty in getting pepper.js to v1.0.

If an unimplemented interface is requested, pepper.js will return a null pointer
and log the request to the JavaScript console.  If an unimplemented function is
called, an exception with be thrown.

To find which interfaces have been implemented, run the following command in the
root of the repo:

::

    git grep "registerInterface(\""

To find unimplemented functions:

::

    git grep "not implemented"

If you need a particular interface or function for your application, do not
hesitate to file a feature request on the bug tracker.  Test cases and patches
are welcome, if you're particularly interested in the feature.

Implementation Errata
---------------------

The Graphics2D and Graphics3D interfaces will automatically swap buffers every
frame, even if Flush or SwapBuffers is not called. This behavior should not be
noticeable for most applications. Explicit swapping could be emulated by
creating an offscreen buffer, but this would cost time and memory.

Graphics3D may not strictly honor ``PP_GRAPHICS3DATTRIB_*`` parameters but best
effort will be made to do something reasonable.  WebGL_ provides less control
than PPAPI, and pepper.js is implemented on top of WebGL.  For example, if a
24-bit depth buffer is requested there will be a depth buffer but WebGL only
makes guarantees that depth buffers are at least 16 bits.

.. _WebGL: https://www.khronos.org/registry/webgl/specs/1.0/

In NaCl, ``PPB_View`` specifies coordinates in terms of device independent
pixels (the resolution of your screen, divided by a constant factor for high DPI
displays).  Most DOM elements work in terms of CSS pixels, however, which are
affected by zooming in or out on a page and other forms of full-page scaling.
In effect, NaCl sees the rectangle it occupies on the screen grow and shrink
when the page is scaled.  NaCl can transform from device independent pixels to
CSS pixels by using the scaling factor returned from ``GetCSSScale``.  pepper.js
always works in terms of CSS pixels because JavaScript does not appear to expose
such a scaling factor.  ``GetCSSScale`` will always return ``1``.  In effect,
pepper.js does not see the rectangle it occupies change when zooming in or out
on a page.

Using BGRA image formats will result in a silent performance penalty. In
general, web APIs tend to be strongly opinionated that premultiplied RGBA is the
image format that should be used. Any other format must be manually converted
into premultiplied RGBA.

The Audio API only supports one sample rate - whatever the underlying Web Audio
API uses, which is whatever the OS defaults to, which tends to be either 44.1k
or 48k. 48k appears to be a little more common.  This means that an app
expecting a particular sample rate may not be able to get it, and this can cause
serious difficulties.  In the future, resampling could be performed as a
polyfill, but this would be slow.

URLLoader intentionally deviates from the native implementation's behavior when
it is at odds with XMLHttpRequest. For example, pepper.js does not identify CORS
failures as ``PP_ERROR_NOACCESS``, instead it returns ``PP_ERROR_FAILED``.

URLLoader does not stream - the data appears all at once. This is a consequence
of doing an XHR with ``requestType`` set to ``arraybuffer``, it does not appear
to give partial results.

If multiple mouse buttons are held, pepper.js will list all of them as event
modifiers. PPAPI will only list one button - the one with the lowest enum
value. There is a known bug where pepper.js will not update the modifier state
if a button is pressed or released outside of pepper.js's canvas.

Platform Errata
---------------

``PPB_Graphics3D`` does not work on Internet Explorer 10 or before because WebGL
is not supported.  IE11 supports WebGL to some extent, but it still has a way to
go before it is considered a conformant implementation of WebGL 1.0.  It is
missing features such as bufferSubData and accepting arrays of byte indices when
drawing elements.  If you want a 3D app to work on IE11, you must test it on
IE11 and find workarounds for missing features.  WebGL is supported on Safari,
but it must be manually enabled: https://discussions.apple.com/thread/3300585.

``PPB_MouseLock`` and ``PPB_Fullscreen`` are only supported in Chrome and
Firefox.  The behavior of these interfaces varies somewhat between the two
browsers, however.  Safari supports fullscreen, but does not support mouse lock.

The file interfaces are currently supported only by Chrome. (Creation and last
access time are not supported, even on Chrome.) A polyfill for Firefox and IE is
included in pepper.js, but it has a few known bugs - such as not being able to
resize existing files. Another issue is that the Closure compiler will rename
fields in persistent data structures, resulting in data incompatibility/loss
between Debug and Release versions, and possibly even between different Release
versions.

Chrome will smoothly scale the image composited into the page when using
pepper.js, all other browsers will do nearest-neighbor scaling.  Native Client
executables will do nearest-neighbor scaling in Chrome.  This means low res or
pixel style graphics will be slightly blurred on Chrome with pepper.js, unless
the back buffer is the same size as the view port and the scaling factor for
high DPI displays is accounted for.

Input events are a little fiddly due to inconsistencies between browsers. For
example, the delta for scroll wheel events is scaled differently in different
browsers. pepper.js attempts to normalize this, but in general, cross-platform
inconsistencies should be expected in the input event interface.

Mobile browsers have not been tested.

The "Probe Interfaces" example should help discover what interfaces are
available on a particular platform.

------------
Getting Help
------------

* native-client-discuss_ for questions about pepper.js and Native Client.
* emscripten-discuss_ for Emscripten-specific questions.

.. _native-client-discuss: https://groups.google.com/forum/#!forum/native-client-discuss
.. _emscripten-discuss: https://groups.google.com/forum/#!forum/emscripten-discuss
