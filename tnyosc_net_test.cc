
#include "tnyosc.hpp"
#include <boost/asio.hpp>

#define HOST ("127.0.0.1")
#define PORT ("7400")

using boost::asio::ip::udp;

tnyosc::Message::Ptr create_osc_message()
{
  // create a OSC messsage with address "/test". if no argument is given to
  // the constructor, default is "/tnyosc".
  tnyosc::Message::Ptr msg(new tnyosc::Message("/test"));
  
  // append a OSC-string
  msg->append("hello tnyosc");
  
  // append some ints and floats
  for (int i = 0; i < 5; i++) {
    // int32
    msg->append(i);
    // float32
    msg->append(i*2.0f);
  }

  return msg;
}

int main(int argc, const char* argv[])
{
  try {
    // boost::asio library for sending UDP packets
    boost::asio::io_service io_service;
    udp::socket socket(io_service, udp::endpoint(udp::v4(), 0));
    udp::resolver resolver(io_service);
    udp::resolver::query query(udp::v4(), HOST, PORT);
    udp::resolver::iterator iterator = resolver.resolve(query);

    // create a OSC message
    tnyosc::Message::Ptr msg = create_osc_message();
    
    // create a OSC bundle
    tnyosc::Bundle::Ptr bundle(new tnyosc::Bundle());

    // add the OSC message to the bundle
    bundle->append(msg);

    // you can also add a bundle to itself. this works because the data is
    // copied when the function is called. 
    bundle->append(bundle);

    // send the message over UDP
    socket.send_to(boost::asio::buffer(bundle->data(), bundle->size()), *iterator);
  } catch (std::exception& e) {
    std::cerr << "Excetion: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}

