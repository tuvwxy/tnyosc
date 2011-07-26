# tnyosc

## Tiny Open Sound Control Library

tnyosc is a header-only Open Sound Control library written in C++ for creating OSC-compliant messages. tnyosc supports Open Sound Control 1.0 and 1.1 types and other nonstandard types, and bundles.

This has been tested on OS X 10.6 and Linux (CentOS). It should work with any POSIX systems. Windows support is on it's way but I can't tell you when it would be out since I don't have a need for it yet. I can put more effort in it if anyone is interested though!

To use the library, you just need "tnyosc.hpp" header file.

Here's an example of creating a OSC message and inserting it in a bundle:

    // create a OSC message with OSC Address "/test"
    tnyosc::Message msg("/test");

    // add a few arguments
    msg.append(1000); // int32 type
    msg.append(1.0f); // float32 type
    msg.append("hello tnyosc"); // OSC-string type

    // Messages can be bundled easily
    tnyosc::Bundle bundle;
    bundle.append(msg);

To access the buffer as a unsigned char array and get its size:

    tnyosc::Message msg;
    unsigned char* data = msg.data();
    size_t size = msg.size();

You can call the same functions for bundle:

    tnyosc::Bundle bundle;
    unsigned char* data = bundle.data();
    size_t size = bundle.size();

Here's a complete example using boost::asio to send a OSC message over UDP:

    #include "tnyosc.hpp"
    #include <boost/asio.hpp>

    #define HOST ("127.0.0.1")
    #defin PORT ("7400")

    using boost::asio::ip:udp;

    int main(int argc, const char* argv[])
    {
      // boost::asio library for sending UDP packets
      boost::asio::io_service io_service;
      udp::socket socket(io_service, udp::endpoint(udp::v4(), 0));
      udp::resolver resolver(io_service);
      udp::resolver::query query(udp::v4(), HOST, PORT);
      udp::resolver::iterator iterator = resolver.resolve(query);

      // create a OSC message
      tnyosc::Message msg("/test");
      msg.append("hello tnyosc");

      // send the message 
      socket.send_to(boost::asio::buffer(msg.data(), msg.size()), *iterator);

      return 0;
    }

A similar example is inside tnyosc\_net\_tesc\_.cc.

## BSD-License

Copyright (c) 2011 Toshiro Yamada

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


