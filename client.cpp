#include <iostream>
#include "RtMidi.h"
#include <deque>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "signal_handler.hpp"
#include "json.hpp"
#include <memory>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

enum {
  max_length = 1024
};

using boost::asio::ip::tcp;
typedef std::deque<nlohmann::json> midi_message_queue;

class chat_client {
 public:
  chat_client(boost::asio::io_context &io_context,
              const tcp::resolver::results_type &endpoints)
      : io_context_(io_context),
        socket_(io_context) {
    assign_uuid();
    do_connect(endpoints);
  }
  void assign_uuid() {
    std::stringstream uuid_stream;
    uuid_stream << boost::uuids::random_generator()();
    uuid = uuid_stream.str();
  }
  void init_midi() {
    midiin->openVirtualPort("midiPool send");
    midiin->ignoreTypes(false, false, false);
    midiout->openVirtualPort("midiPool receiver");
    midiin->ignoreTypes(false, false, false);
  }
  void init_callback(RtMidiIn::RtMidiCallback send_callbackPointer, void *client) {
    midiin->setCallback(send_callbackPointer, client);
  }

  void write(const nlohmann::json &msg) {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress && connected) {
      do_write();
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
                                     do_write();
                                   }
                                   do_read();
                                 }
                               });
  }

  void do_read() {
    boost::asio::async_read(socket_, response_,
                            boost::asio::transfer_at_least(1),
                            boost::bind(&chat_client::handle_read_content, this,
                                        boost::asio::placeholders::error));
  }

  void resetResponse() {
    response_.commit(response_.size());
  }

  void handle_read_content(const boost::system::error_code &err) {
    if (!err) {
      auto data = response_.data();
      std::string str(boost::asio::buffers_begin(data),
                      boost::asio::buffers_begin(data) + data.size());
      response_.consume(data.size());
      std::cout << "response" << str << std::endl;
      nlohmann::json cur_message = nlohmann::json::parse(str);
      std::cout << "response received uuid:" << cur_message["meta"]["uuid"];
      std::vector<unsigned char> message;
      message.clear();
      message.push_back(cur_message["bytes"][0]);
      midiout->sendMessage(&message);
      std::cout << "MIDI start" << std::endl;
      do_read();
    } else if (err != boost::asio::error::eof) {
      std::cout << "Error: " << err << "\n";
    }
  }
  const std::string &GetUuid() const {
    return uuid;
  }
 private:
  void do_write() {
    boost::asio::async_write(socket_, boost::asio::buffer(write_msgs_.front().dump(),
                                                          write_msgs_.front().dump().size()),
                             boost::bind(&chat_client::handle_write, this, _1));
    write_msgs_.pop_front();
    if (!write_msgs_.empty()) {
      do_write();
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
  std::string uuid;
  boost::asio::streambuf response_;
  midi_message_queue write_msgs_;
  RtMidiIn *midiin = new RtMidiIn();
  RtMidiOut *midiout = new RtMidiOut();
};

void midi_callback(double deltatime, std::vector<unsigned char> *message, void *userData) {
  int nBytes = message->size();
  if (nBytes > 0) {
    chat_client *client = (chat_client *) userData;
    nlohmann::json j;
    for (int i = 0; i < nBytes; i++) {
      j["bytes"][i] = (int) message->data()[i];
    }
    j["meta"]["timestamp"] = deltatime;
    j["meta"]["uuid"] = client->GetUuid();
//    client->write(j);
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
    io_context.run();
  }
  catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  return 0;
}
