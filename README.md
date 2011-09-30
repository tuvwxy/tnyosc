# tnyosc

## Tiny Open Sound Control Library

tnyosc is a tiny Open Sound Control library written in C++ for creating and parsing (or dispatching) OSC-compliant messages. tnyosc supports Open Sound Control 1.0 and 1.1 types and other nonstandard types, and bundles.

This has been tested on OS X 10.6 and Linux (CentOS). It should work with any POSIX systems. Windows support is on it's way but I can't tell you when it would be out since I don't have a need for it yet. I can put more effort in it if anyone is interested though!

To use the library to just create and send Open Sound Control message, you just need `tnyosc.hpp` header file.

If you're interested in parsing or dispatching received OSC messages, you need both `tnyosc.hpp` and `tnyosc-dispatch.hpp` headers and `tnyosc-dispatch.cc` source files.

## tnyosc Example

### Creating and Sending OSC Messages

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

To access the buffer as a char array and get its size:

    tnyosc::Message msg;
    char* data = msg.data();
    size_t size = msg.size();

You can call the same functions for bundle:

    tnyosc::Bundle bundle;
    char* data = bundle.data();
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

A similar example is inside `tnyosc_net_test.cc`.

### Dispatching OSC Messages

`tnyosc-dispatch.hpp` and `tnyosc-dispatch.cc` include code for dispatching received OSC messages. It is designed so that it does not enforce particular threading model and user have more control over how to organize their code.

Dispatching OSC message is a little more involved because OSC methods (aka callback functions) need to be registered and OSC messages need to be parsed and matched against the methods.

First, we need to define a OSC method. The method has a signature:

    void method(const std::string& address, 
                const std::vector<tnyosc::Argument>& argv,
                void* user_data);

`address` is the OSC Address, which looks like a URL.
`argv` is arguments in the OSC message.
`user_data` is user specified pointer to a data that was set when registering the method.

We can then add this method to the `Dispatcher` class with the matching signature.

    Dispatcher dispatcher;
    dispatcher.add_method("/match/address", /* match exactly with this OSC address */
                          NULL,             /* no arguments specified */
                          &method,          /* pointer to the OSC method */
                          NULL);            /* no user data specified */

Then, once a raw OSC message is received, it can invoke the method by

    char* msg_data;
    size_t msg_size;

    // ... fill msg_data and msg_size with received raw OSC message

    // Get a matched callback list
    std::list<CallbackRef> callback_list = dispatcher.match_methods(msg_data, msg_size);

    // We can iterate through the list and invoke all OSC methods
    std::list<CallbackRef>::iterator it = callback_list.begin();
    for (; it != callback_list.end(); ++it) {
      (*it)->method(((*it)->address, (*it)->argv, (*it)->user_data);
    }

A full example can be found in `tnyosc-dispatch_test.cc`.

## BSD-License

Copyright (c) 2011 Toshiro Yamada

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


