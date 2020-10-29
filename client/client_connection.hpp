//
// Created by my5t3ry on 10/13/20.
//

#ifndef MIDIPOOL__CHAT_CLIENT_HPP_
#define MIDIPOOL__CHAT_CLIENT_HPP_
#include "utils/common.hpp"
#include "midi/midi_cue.hpp"

class client_connection :
    public std::enable_shared_from_this<client_connection> {
 public:
  client_connection(boost::asio::io_context &io_context,
                    const tcp::resolver::results_type &endpoints)
      : socket_(io_context),
        timer_(socket_.get_executor()) {
    assign_uuid();
    do_connect(endpoints);
  }
  void assign_uuid() {
    std::stringstream uuid_stream;
    uuid_stream << boost::uuids::random_generator()();
    uuid = uuid_stream.str();
  }
  const std::string &GetUuid() const {
    return uuid;
  }

  void SetMidiCue(midi_cue *midi_cue) {
    client_connection::midi_cue = midi_cue;
  }

  void do_connect(const tcp::resolver::results_type &endpoints) {
    boost::asio::async_connect(socket_, endpoints,
                               [this](boost::system::error_code ec, tcp::endpoint endpoint) {
                                 if (!ec) {
                                   co_spawn(socket_.get_executor(),
                                            [this] { return this->reader(); },
                                            detached);
                                   server_ip = boost::lexical_cast<std::string>(socket_.remote_endpoint().address());
                                   LOG(INFO) << "client connected to: " << server_ip << ":"
                                             << socket_.remote_endpoint().port();
                                 } else {
                                   LOG(ERROR) << "connect failed: " << ec.message();
                                 }
                               });
  }

  awaitable<void> reader() {
    while (socket_.is_open()) {
      try {
        std::string read_msg;
        std::size_t n = co_await boost::asio::async_read_until(socket_,
                                                               boost::asio::dynamic_buffer(read_msg, 2096),
                                                               "\n",
                                                               use_awaitable);
        if (n > 0) {
          nlohmann::json cur_message = nlohmann::json::parse(read_msg.substr(0, n));
          vector<unsigned char> message_bytes;
          message_bytes.clear();
          for (unsigned char cur_byte: cur_message["bytes"]) {
            message_bytes.push_back(cur_byte);
          }
          midi_message midi_message = midi_cue->build_midi_message(&message_bytes,
                                                                   (long) cur_message["meta"]["exec_timestamp"],
                                                                   (int) cur_message["meta"]["clock_rate"]);
          midi_cue->cue_midi_message(midi_message);
          LOG(DEBUG) << "adding midi message:" << cur_message.dump();
          read_msg.erase(0, n);
          SLEEP(10);
        }
      } catch (std::exception &e) {
        LOG(ERROR) << e.what();
        //        stop();
      }
    }
  }

  awaitable<void> writer() {
    try {
      writing = 1;
      while (!midi_cue->write_msgs_.empty()) {
        co_await boost::asio::async_write(socket_,
                                          boost::asio::buffer(midi_cue->write_msgs_.front().dump()), use_awaitable);
        co_await boost::asio::async_write(socket_,
                                          boost::asio::buffer("\n"), use_awaitable);
        midi_cue->write_msgs_.pop_front();
      }
      writing = 0;
    }
    catch (std::exception &e) {
      LOG(ERROR) << e.what();
      stop();
    }
  }
  void stop() {
    socket_.close();
  }
 private:
  tcp::socket socket_;
  midi_cue *midi_cue;
  std::string server_ip;
  boost::asio::steady_timer timer_;
  std::string uuid;
  int writing = 0;
};

#endif //MIDIPOOL__CHAT_CLIENT_HPP_
