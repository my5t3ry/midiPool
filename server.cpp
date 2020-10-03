#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include "signal_handler.hpp"
#include <utility>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "json.hpp"
#include <boost/thread.hpp>

using boost::thread;
using boost::mutex;

mutex a;
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#if defined(WIN32)
#include <windows.h>
#define SLEEP( milliseconds ) Sleep( (DWORD) milliseconds )
#else // Unix variants
#include <unistd.h>
#define SLEEP(milliseconds) usleep( (unsigned long) (milliseconds * 1000.0) )
#endif

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
    assign_uuid();
  }
  void assign_uuid() {
    std::stringstream uuid_stream;
    uuid_stream << boost::uuids::random_generator()();
    uuid = uuid_stream.str();
  }
  const std::string &GetUuid() const {
    return uuid;
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
                            boost::bind(&chat_session::handle_read_content, self,
                                        boost::asio::placeholders::error));
  }
  void resetResponse() {
    response_.commit(response_.size());
    do_read();
  }

  void handle_read_content(const boost::system::error_code &err) {
    if (!err) {
      std::stringstream cur_stream;
      cur_stream << &response_;
      std::cout << cur_stream.str() << std::endl;
      nlohmann::json cur_message = nlohmann::json::parse(cur_stream.str());
      resetResponse();
      deliver(cur_message);
    } else if (err != boost::asio::error::eof) {
      std::cout << "Error: " << err << "\n";
    }
  }

  void do_write() {
    auto self(shared_from_this());
    if (!write_msgs_.empty()) {
      std::cout << "sending:" << write_msgs_.front().dump();
      boost::asio::async_write(socket_, boost::asio::buffer(write_msgs_.front().dump(),
                                                            write_msgs_.front().dump().size()),
                               boost::bind(&chat_session::handle_write, self,
                                           boost::asio::placeholders::error));
    }
  }

  void handle_write(const boost::system::error_code &ec) {
    if (!ec) {
      write_msgs_.pop_front();
      if (!write_msgs_.empty()) {
        SLEEP(30);
        do_write();
      }
    } else {
      std::cout << "Error on heartbeat: " << ec.message() << "\n";
    }
  }

  tcp::socket socket_;
  chat_room &room_;
  boost::asio::streambuf response_;
  std::string uuid;
  midi_message_queue write_msgs_;
};

void midiClock(int sleep_ms, std::shared_ptr<chat_session> session) {
  a.lock();
  int k = 0, j = 0;
  std::cout << "Generating clock at "
            << (60.0 / 24.0 / sleep_ms * 1000.0)
            << " BPM." << std::endl;

  // Send out a series of MIDI clock messages.
  // MIDI start
  nlohmann::json message;
  message["bytes"][0] = 0xFA;
  message["meta"]["uuid"] = session->GetUuid();

  std::cout << "MIDI start" << std::endl;
  session->deliver(message);

  while (true) {
    if (j > 0) {
      // MIDI continue
      nlohmann::json message;
      message["bytes"][0] = 0xFB;
      message["meta"]["uuid"] = session->GetUuid();
      session->deliver(message);
      std::cout << "MIDI continue" << std::endl;
    }

    for (k = 0; k < 96; k++) {
      // MIDI clock
      nlohmann::json message;
      message["bytes"][0] = 0xF8;
      message["meta"]["uuid"] = session->GetUuid();

      session->deliver(message);
      if (k % 24 == 0)
        std::cout << "MIDI clock (one beat)" << std::endl;
      SLEEP(sleep_ms);
    }

    // MIDI stop
    nlohmann::json message;
    message["bytes"][0] = 0xFC;
    message["meta"]["uuid"] = session->GetUuid();

    session->deliver(message);
    std::cout << "MIDI stop" << std::endl;
    SLEEP(500);
  }
  a.unlock();
}

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
            const std::shared_ptr<chat_session> &session = std::make_shared<chat_session>(std::move(socket), room_);
            thread *midiThread = new thread(midiClock, 60, session);
            session->start();
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
