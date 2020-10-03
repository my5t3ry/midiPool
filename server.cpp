#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "json.hpp"


class chat_participant {
 public:
  virtual ~chat_participant() {}
  virtual void deliver(const nlohmann::json &msg) = 0;
};

using boost::asio::ip::tcp;
typedef std::deque<nlohmann::json> midi_message_queue;
typedef std::shared_ptr<chat_participant> chat_participant_ptr;

class chat_room {
 public:
  void join(chat_participant_ptr participant) {
    participants_.insert(participant);
    for (auto msg: recent_msgs_)
      participant->deliver(msg);
  }

  void leave(chat_participant_ptr participant) {
    participants_.erase(participant);
  }

  void deliver(const nlohmann::json &msg) {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
      recent_msgs_.pop_front();

    for (auto participant: participants_)
      participant->deliver(msg);
  }

 private:
  std::set<chat_participant_ptr> participants_;
  enum {
    max_recent_msgs = 100
  };
  midi_message_queue recent_msgs_;
};

class chat_session
    : public chat_participant,
      public std::enable_shared_from_this<chat_session> {
 public:
  chat_session(tcp::socket socket, chat_room &room)
      : socket_(std::move(socket)),
        room_(room) {
  }

  void start() {
    room_.join(shared_from_this());
    do_read();
  }

  void deliver(const nlohmann::json &msg) {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress) {
      do_write();
    }
  }

 private:
  void do_read() {
    auto self(shared_from_this());
    boost::asio::async_read(socket_, response_,
                            boost::asio::transfer_at_least(1),
                            boost::bind(&chat_session::handle_read_content, this,
                                        boost::asio::placeholders::error));
  }

  void handle_read_content(const boost::system::error_code &err) {
    if (!err) {
      std::cout << &response_ << std::endl;
      boost::asio::async_read(socket_, response_,
                              boost::asio::transfer_at_least(1),
                              boost::bind(&chat_session::handle_read_content, this,
                                          boost::asio::placeholders::error));
    } else if (err != boost::asio::error::eof) {
      std::cout << "Error: " << err << "\n";
    }
  }

  void do_write() {
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
                             boost::asio::buffer(write_msgs_.front().dump()),
                             [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                               if (!ec) {
                                 write_msgs_.pop_front();
                                 if (!write_msgs_.empty()) {
                                   do_write();
                                 }
                               } else {
                                 room_.leave(shared_from_this());
                               }
                             });
  }

  tcp::socket socket_;
  chat_room &room_;
  boost::asio::streambuf response_;
  midi_message_queue write_msgs_;
};

class chat_server {
 public:
  chat_server(boost::asio::io_context &io_context,
              const tcp::endpoint &endpoint)
      : acceptor_(io_context, endpoint) {
    do_accept();
  }
 private:
  void do_accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
          if (!ec) {
            std::make_shared<chat_session>(std::move(socket), room_)->start();
          }

          do_accept();
        });
  }
  tcp::acceptor acceptor_;
  chat_room room_;
};

int main(int argc, char *argv[]) {
  try {
    if (argc < 2) {
      std::cerr << "Usage: chat_server <port> [<port> ...]\n";
      return 1;
    }
    boost::asio::io_context io_context;
    std::list<chat_server> servers;
    for (int i = 1; i < argc; ++i) {
      tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
      servers.emplace_back(io_context, endpoint);
    }
    io_context.run();
  }
  catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  return 0;
}
