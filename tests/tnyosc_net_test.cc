
#include <boost/asio.hpp>
#include <iostream>
#include "tnyosc.hpp"

using boost::asio::ip::udp;
using boost::asio::ip::tcp;

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
  if (argc != 3) {
    std::cout << argv[0] << " HOST PORT\n";
    return -1;
  }

  try {
    // boost::asio library for sending UDP packets
    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    tcp::resolver::query query(tcp::v4(), argv[1], argv[2]);
    tcp::resolver::iterator iterator = resolver.resolve(query);

    tcp::socket socket(io_service, tcp::endpoint(tcp::v4(), 0));
    socket.connect(*iterator);

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
    int32_t send_size = htonl(bundle->size());
    boost::asio::write(socket, boost::asio::buffer(&send_size, 4));
    boost::asio::write(socket, boost::asio::buffer(bundle->data(), bundle->size()));
  } catch (std::exception& e) {
    std::cerr << "Excetion: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}

