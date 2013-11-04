===================
Bullet Physics demo
===================

This demo builds `Bullet Physics`_ library for PNaCl and emscripten, and uses
John McCutchan's `Native Client Acceleration module`_ library to communicate
between JavaScript and the module.

.. _`Bullet Physics`: http://bulletphysics.org
.. _`Native Client Acceleration module`: https://github.com/johnmccutchan/NaClAMBase

-------------
Prerequisites
-------------

Make sure you have the following tools installed:

* autoconf
* automake
* libtool

On Mac OS X, it is probably easiest to use Homebrew_::

    brew install autoconf
    ...

On Linux, you can use your system's package manager, e.g.::

    apt-get install autoconf
    ...

This example does not currently support Windows.

.. _Homebrew: http://brew.sh

--------
Building
--------

To build for PNaCl::

    make TOOLCHAIN=pnacl

To build for emscripten/pepper.js::

    make TOOLCHAIN=emscripten
