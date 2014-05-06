WebSocketWebRTCRecorders
========================

These are modifications of existing recording scripts that allow recording through websockets, not just downloading Blobs


WebSocketServer
===============

The Server implementation is a simple C program that utilizes [libwebsockets](http://libwebsockets.org/trac/libwebsockets) and [gstreamer](http://gstreamer.com/), specifically version 0.10 to get and play the media sent down from the front end javascript.