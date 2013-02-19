#!/usr/bin/python
# Copyright 2008, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


"""A tiny web server.

This is intended to be used for testing, and
only run from within the
googleclient/native_client
"""


import BaseHTTPServer
import logging
import os
import SimpleHTTPServer
import SocketServer
import sys

logging.getLogger().setLevel(logging.INFO)

# Using 'localhost' means that we only accept connections
# via the loop back interface.
SERVER_PORT = 5103
SERVER_HOST = ''

# We only run from the native_client directory, so that not too much
# is exposed via this HTTP server.  Everything in the directory is
# served, so there should never be anything potentially sensitive in
# the serving directory, especially if the machine might be a
# multi-user machine and not all users are trusted.  We only serve via
# the loopback interface.

# the sole purpose of this class is to make the BaseHTTPServer threaded
class ThreadedServer(SocketServer.ThreadingMixIn,
                     BaseHTTPServer.HTTPServer):
  pass


def Run(server_address,
        server_class=ThreadedServer,
        handler_class=SimpleHTTPServer.SimpleHTTPRequestHandler):
  httpd = server_class(server_address, handler_class)
  logging.info('started server on port %d', httpd.server_address[1])
  httpd.serve_forever()
# enddef


if __name__ == '__main__':
  if len(sys.argv) > 1:
    Run((SERVER_HOST, int(sys.argv[1])))
  else:
    Run((SERVER_HOST, SERVER_PORT))
  # endif
# endif
