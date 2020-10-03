#include <cstring>
#include <iostream>
#include "RtMidi.h"
#include <deque>
#include <thread>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "main_loop.hpp"
#include "json.hpp"


using boost::asio::ip::tcp;

enum {
  max_length = 1024
};

typedef std::deque<nlohmann::json> midi_message_queue;

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

  void write(const nlohmann::json &msg) {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress && connected) {
      do_write(write_msgs_.front());
    }
  }

  void close() {
    boost::asio::post(io_context_, [this]() { socket_.close(); });
  }

  void do_connect(const tcp::resolver::results_type &endpoints) {
    boost::asio::async_connect(socket_, endpoints,
                               [this](boost::system::error_code ec, tcp::endpoint) {
                                 if (!ec) {
                                   connected = 1;
                                   if (!write_msgs_.empty()) {
                                     do_write(write_msgs_.front());
                                   }
                                 }
                               });
  }

 private:
  void do_write(nlohmann::json data) {
    std::string dump = data.dump();
    boost::asio::streambuf buf;
    std::ostream str(&buf);
    str << dump;
    std::cout << dump << std::endl;
    boost::asio::async_write(socket_, buf,
                             boost::bind(&chat_client::handle_write, this, _1));
    write_msgs_.pop_front();
    if (!write_msgs_.empty()) {
      if (write_msgs_.front()) {
        do_write(write_msgs_.front());
      }
    }
  }
  void handle_write(const boost::system::error_code &ec) {
    if (!ec) {
      // Wait 10 seconds before sending the next heartbeat.

    } else {
      std::cout << "Error on heartbeat: " << ec.message() << "\n";
    }
  }

 private:
  boost::asio::io_context &io_context_;
  tcp::socket socket_;
  int connected = 0;
  midi_message_queue write_msgs_;
  RtMidiIn *midiin = new RtMidiIn();
};

void midi_callback(double deltatime, std::vector<unsigned char> *message, void *userData) {
  int nBytes = message->size();
  if (nBytes > 0) {
    nlohmann::json j;
    for (int i = 0; i < nBytes; i++) {
      j["bytes"][i] = (int) message->data()[i];
    }
    j["meta"]["timestamp"] = deltatime;
    ((chat_client *) userData)->write(j);
  }
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
    c.init_callback(&midi_callback, &c);
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
