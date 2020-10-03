#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <cstring>
#include <iostream>
#include "RtMidi.h"
#include <deque>
#include <thread>
#include <boost/asio.hpp>
#include "midi_message.hpp"
#include "main_loop.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


// Short alias for this namespace
namespace pt = boost::property_tree;

using boost::asio::ip::tcp;

enum {
  max_length = 1024
};

typedef std::deque<pt::ptree> midi_message_queue;

class chat_client {
 public:
  chat_client(boost::asio::io_context &io_context,
              const tcp::resolver::results_type &endpoints)
      : io_context_(io_context),
        socket_(io_context) {
    do_connect(endpoints);
  }

  void init_midi() {
    midiin->openVirtualPort("test");
    midiin->ignoreTypes(false, false, false);
  }

  void init_callback(RtMidiIn::RtMidiCallback callbackPointer, void *client) {
    midiin->setCallback(callbackPointer, client);
  }

  void write(const pt::ptree &msg) {
    bool write_in_progress = !write_msgs_.empty();
    if (write_in_progress) {
      do_write(write_msgs_.front());
    } else {
      write_msgs_.push_back(msg);
    }
  }

  void close() {
    boost::asio::post(io_context_, [this]() { socket_.close(); });
  }

 private:
  void do_connect(const tcp::resolver::results_type &endpoints) {
    boost::asio::async_connect(socket_, endpoints,
                               [this](boost::system::error_code ec, tcp::endpoint) {
                                 if (!ec) {
                                   do_read_header();
                                 }
                               });
  }

  void do_read_header() {
//    boost::asio::async_read(socket_,
//                            boost::asio::buffer(read_msg_.data(), midi_message::header_length),
//                            [this](boost::system::error_code ec, std::size_t /*length*/) {
//                              if (!ec && read_msg_.decode_header()) {
//                                do_read_body();
//                              } else {
//                                socket_.close();
//                              }
//                            });
  }

  void do_read_body() {
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                            [this](boost::system::error_code ec, std::size_t /*length*/) {
                              if (!ec) {
                                std::cout.write(read_msg_.body(), read_msg_.body_length());
                                std::cout << "\n";
                                do_read_header();
                              } else {
                                socket_.close();
                              }
                            });
  }

  int do_write(pt::ptree data) {
    boost::asio::streambuf buf;
    std::ostream str(&buf);
    pt::write_json(str, data);
    pt::write_json(std::cout, data);
    boost::asio::async_write(socket_,
                             buf,
                             [this](boost::system::error_code ec, std::size_t) {
                               if (!ec) {
                                 if (!write_msgs_.empty()) {
                                   do_write(write_msgs_.front());
                                   write_msgs_.pop_front();
                                 }
                               } else {
                                 socket_.close();
                               }
                             });
    return 1;
  }

 private:
  boost::asio::io_context &io_context_;
  tcp::socket socket_;
  midi_message read_msg_;
  midi_message_queue write_msgs_;
  RtMidiIn *midiin = new RtMidiIn();
};

void callback(double deltatime, std::vector<unsigned char> *message, void *userData) {

  int nBytes = message->size();
  pt::ptree data;
  pt::ptree byte_nodes;
  pt::ptree meta_nodes;

  for (int i = 0; i < nBytes; i++) {
    std::cout << "Byte " << i << " = " << (int) message->data()[i] << ", ";
    std::string str = "byte" + std::to_string(i);
    std::vector<char> charStr(str.c_str(), str.c_str() + str.size() + 1);
    byte_nodes.push_front(pt::ptree::value_type(str, std::to_string(message->data()[1])));
  }

  if (nBytes > 0) {
    std::cout << "stamp = " << deltatime << std::endl;
    pt::ptree timestamp_node;
    timestamp_node.put("timestamp", deltatime);
    meta_nodes.push_back(pt::ptree::value_type("", timestamp_node));
  }
  data.put_child("bytes", byte_nodes);
  data.put_child("meta", meta_nodes);
  ((chat_client *) userData)->write(data);
}

int main(int argc, char *argv[]) {
  try {
    if (argc != 3) {
      std::cerr << "Usage: chat_client <host> <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[1], argv[2]);
    static chat_client c(io_context, endpoints);
    c.init_midi();
    c.init_callback(&callback, &c);

    std::thread t([&io_context]() { io_context.run(); });
    Shutdown shutdown;
    shutdown.init();
    while (true) {
      sleep(1);
    }
  }
  catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
