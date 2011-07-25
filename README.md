# tnyosc

## Tiny Open Sound Control Library

tnyosc is a header-only Open Sound Control library written in C++ for creating OSC-compliant messages. tnyosc supports Open Sound Control 1.0 and 1.1 types and other nonstandard types, and bundles.

Here's an example of creating a OSC message and bundle:

    // create a OSC message with OSC Address "/test"
    tnyosc::Message msg("/test");

    // add a few arguments
    msg.append(1000); // int32 type
    msg.append(1.0f); // float32 type
    msg.append("hello tnyosc"); // OSC-string type

    // Messages can be bundled easily
    tnyosc::Bundle bundle;
    bundle.append(msg);

This is another example using boost::asio to send a OSC message over UDP:

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

