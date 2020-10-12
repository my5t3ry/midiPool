#include "common.hpp"
#include <cstdlib>
#include <set>
#include <chrono>

#if defined(WIN32)
#include <windows.h>
#define SLEEP( milliseconds ) Sleep( (DWORD) milliseconds )
#else // Unix variants
#include <unistd.h>
#include <asoundlib.h>
#define SLEEP(milliseconds) usleep( (unsigned long) (milliseconds * 1000.0) )
#endif

using boost::thread;
using boost::mutex;
using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::redirect_error;
using boost::asio::use_awaitable;
namespace pt = boost::posix_time;


class chat_participant {
 public:
  virtual ~chat_participant() {}
  virtual void deliver(nlohmann::json &msg) = 0;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

class chat_room {
 public:
  void join(chat_participant_ptr participant) {
    participants_.insert(participant);
//    for (auto msg: recent_msgs_)
//      participant->deliver(msg);
  }

  void leave(chat_participant_ptr participant) {
    participants_.erase(participant);
  }

  void deliver(nlohmann::json msg) {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
      recent_msgs_.pop_front();

    for (auto participant: participants_)
      participant->deliver(msg);
  }

 private:
  std::set<chat_participant_ptr> participants_;
  enum { max_recent_msgs = 100 };
  std::deque<std::string> recent_msgs_;
};

class chat_session
    : public chat_participant,
      public std::enable_shared_from_this<chat_session> {
 public:
  chat_session(tcp::socket socket, chat_room &room)
      : socket_(std::move(socket)),
        timer_(socket_.get_executor()),
        room_(room) {
    timer_.expires_at(std::chrono::steady_clock::time_point::max());
  }
  time_t get_current_timestamp(){
    pt::ptime current_date_microseconds = pt::microsec_clock::local_time();
    boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
    boost::posix_time::time_duration x = current_date_microseconds - epoch;  // needs UTC to local corr.,
    // but we get the fract. sec.
    struct tm t = boost::posix_time::to_tm(current_date_microseconds);       // this helps with UTC conve.
    return mktime(&t) + 1.0 * x.fractional_seconds() / x.ticks_per_second();
  }


  void start() {
    room_.join(shared_from_this());
    assign_uuid();
    co_spawn(socket_.get_executor(),
             [self = shared_from_this()] { return self->reader(); },
             detached);

    co_spawn(socket_.get_executor(),
             [self = shared_from_this()] { return self->writer(); },
             detached);
  }
  void assign_uuid() {
    std::stringstream uuid_stream;
    uuid_stream << boost::uuids::random_generator()();
    uuid = uuid_stream.str();
  }
  const std::string &get_uuid() const {
    return uuid;
  }

  void deliver(nlohmann::json &msg) {
    write_msgs_.push_back(msg);
    timer_.cancel_one();
  }

 private:
  awaitable<void> reader() {
    while (socket_.is_open()) {
      try {
        std::string read_msg;
        std::size_t n = co_await boost::asio::async_read_until(socket_,
                                                               boost::asio::dynamic_buffer(read_msg, 2048),
                                                               "\n",
                                                               use_awaitable);
        nlohmann::json cur_message = nlohmann::json::parse(read_msg.data());
        if (n > 0) {
          room_.deliver(cur_message);
          read_msg.erase(0, n);
        }
      }
      catch (std::exception &e) {
        LOG(ERROR) << e.what();
        stop();
      }
    }
  }

  awaitable<void> writer() {
    try {
      while (socket_.is_open()) {
        if (write_msgs_.empty()) {
          boost::system::error_code ec;
          co_await timer_.async_wait(redirect_error(use_awaitable, ec));
        } else {
          co_await boost::asio::async_write(socket_,
                                            boost::asio::buffer(write_msgs_.front().dump()), use_awaitable);
          co_await boost::asio::async_write(socket_,
                                            boost::asio::buffer("\n"), use_awaitable);
          write_msgs_.pop_front();
          SLEEP(20);
        }
      }
    }
    catch (std::exception &) {
      stop();
    }
  }

  void stop() {
    room_.leave(shared_from_this());
    socket_.close();
    timer_.cancel();
  }

  tcp::socket socket_;
  boost::asio::steady_timer timer_;
  chat_room &room_;
  std::string uuid;
  std::deque<nlohmann::json> write_msgs_;
};

void midi_clock(int sleep_ms, std::shared_ptr<chat_session> session) {
  int k = 0, four_bars = 0;
  LOG(DEBUG) << "Generating clock at "
             << (60.0 / 24.0 / sleep_ms * 1000.0)
             << " BPM.";

  int num_four_bars = 8;
  while (true) {
    if (four_bars == num_four_bars) {
      nlohmann::json message;
      message["bytes"][0] = MIDI_CMD_COMMON_START;
      message["meta"]["uuid"] = session->get_uuid();
      message["meta"]["exec_timestamp"] = session->get_current_timestamp();
      LOG(DEBUG) << "MIDI start";
      session->deliver(message);
      four_bars = 0;
      message["bytes"][0] = MIDI_CMD_COMMON_STOP;
      message["meta"]["uuid"] = session->get_uuid();
      message["meta"]["exec_timestamp"] = session->get_current_timestamp();
      session->deliver(message);
      LOG(DEBUG) << "MIDI stop";
    }
    if (four_bars > 0) {
      // MIDI continue
      nlohmann::json message;
      message["bytes"][0] = MIDI_CMD_COMMON_CONTINUE;
      message["meta"]["uuid"] = session->get_uuid();
      message["meta"]["exec_timestamp"] = session->get_current_timestamp();
      session->deliver(message);
      LOG(DEBUG) << "MIDI continue";
    }

    for (k = 0; k < 96; k++) {
      // MIDI clock
      nlohmann::json message;
      message["bytes"][0] = MIDI_CMD_COMMON_CLOCK;
      message["meta"]["uuid"] = session->get_uuid();
      message["meta"]["exec_timestamp"] = session->get_current_timestamp();


      session->deliver(message);
      if (k % 24 == 0)
        LOG(DEBUG) << "MIDI clock (one beat)";
      SLEEP(sleep_ms);
    }
    nlohmann::json message;

    four_bars = four_bars + 1;
//    SLEEP(500)
  }
}

awaitable<void> listener(tcp::acceptor acceptor) {
  chat_room room;

  for (;;) {
    const std::shared_ptr<chat_session> &session = std::make_shared<chat_session>(
        co_await acceptor.async_accept(use_awaitable),
        room
    );
    boost::thread *midiThread = new boost::thread(midi_clock, 25, session);
    session->start();
  }
}

int main(int argc, char *argv[]) {
  try {
    if (argc < 2) {
      LOG(DEBUG) << "Usage: chat_server <port> [<port> ...]\n";
      return 1;
    }
    boost::asio::io_context io_context(1);

    for (int i = 1; i < argc; ++i) {
      unsigned short port = std::atoi(argv[i]);
      LOG(INFO) << "Server starts at: " << port;
      co_spawn(io_context,
               listener(tcp::acceptor(io_context, {tcp::v4(), port})),
               detached);
    }
    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) { io_context.stop(); });
    io_context.run();
  }
  catch (std::exception &e) {
    LOG(ERROR) << "Exception: " << e.what() << "\n";
  }
  return 0;
}
