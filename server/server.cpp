#include "utils/common.hpp"
#include <cstdlib>
#include <set>
#include <chrono>

#if defined(WIN32)
#include <windows.h>
#define SLEEP( milliseconds ) Sleep( (DWORD) milliseconds )
#else // Unix variants
#include <unistd.h>
#include <audio/audio_server_transmitter.hpp>
#define SLEEP(milliseconds) usleep( (unsigned long) (milliseconds * 1000.0) )
#endif

class chat_participant {
 public:
  virtual ~chat_participant() {}
  virtual void deliver(nlohmann::json &msg) = 0;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

audio_server_socket audio_server(1000, 1001);

class session_room {
 public:
  session_room() { assign_uuid(); }
 public:
  void join(chat_participant_ptr participant) {
    participants_.insert(participant);
//    for (auto msg: recent_msgs_)
//      participant->deliver(msg);
  }

  const std::string &GetUuid() const {
    return uuid;
  }

  void leave(chat_participant_ptr participant) {
    participants_.erase(participant);
  }

  void deliver(nlohmann::json msg) {
    for (auto participant: participants_)
      participant->deliver(msg);
  }

 private:
  void assign_uuid() {
    std::stringstream uuid_stream;
    uuid_stream << boost::uuids::random_generator()();
    uuid = uuid_stream.str();
  }
  std::set<chat_participant_ptr> participants_;
  std::string uuid;
};

class client_session
    : public chat_participant,
      public std::enable_shared_from_this<client_session> {
 public:
  client_session(tcp::socket socket, session_room &room)
      : socket_(std::move(socket)),
        timer_(socket_.get_executor()),
        client_ip_(boost::lexical_cast<std::string>(socket_.remote_endpoint())),
        room_(room) {
    timer_.expires_at(std::chrono::steady_clock::time_point::max());
    LOG(INFO) << "client has ip:" << client_ip_;
    roc_sender *new_sender = init_sender(client_ip_);
    audio_server.SetSenders(new_sender);
  }

  void start() {
    room_.join(shared_from_this());
    co_spawn(socket_.get_executor(),
             [self = shared_from_this()] { return self->reader(); },
             detached);

    co_spawn(socket_.get_executor(),
             [self = shared_from_this()] { return self->writer(); },
             detached);
  }

  void deliver(nlohmann::json &msg) {
    LOG(DEBUG) << "delivering message:" << msg.dump();
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
  session_room &room_;
  string client_ip_;
  std::deque<nlohmann::json> write_msgs_;
};

void send_start_message(int clock_rate, session_room *room, long midi_buffer);
void midi_clock(int clock_rate, int loop_length, session_room *room, long midi_buffer = 500) {
  int k = 0, four_bars = 0;
  int num_four_bars = loop_length;
  LOG(INFO) << "Generating clock at"
            << " BPM:"
            << (60.0 / 24.0 / clock_rate * 1000.0)
            << " LOOP SIZE:"
            << num_four_bars
            << " MIDI_BUFFER:"
            << midi_buffer;
  send_start_message(clock_rate, room, midi_buffer);
  four_bars = 0;
  while (true) {
    if (four_bars == num_four_bars) {
      nlohmann::json stop_message;
      stop_message["bytes"][0] = MIDI_CMD_COMMON_SONG_POS;
      stop_message["bytes"][1] = 0x00;
      stop_message["meta"]["uuid"] = room->GetUuid();
      stop_message["meta"]["exec_timestamp"] = get_posix_timestamp();
      stop_message["meta"]["clock_rate"] = clock_rate;
      LOG(DEBUG) << "MIDI song pos: 0: " << stop_message.dump();
      room->deliver(stop_message);
      four_bars = 0;
    }
    if (four_bars > 0) {
      nlohmann::json message;
      message["bytes"][0] = MIDI_CMD_COMMON_CONTINUE;
      message["meta"]["uuid"] = room->GetUuid();
      message["meta"]["clock_rate"] = clock_rate;
      message["meta"]["exec_timestamp"] = get_posix_timestamp(midi_buffer);
      room->deliver(message);
      LOG(DEBUG) << "MIDI continue";
    }

    for (k = 0; k < 96; k++) {
      SLEEP(clock_rate);
    }
    nlohmann::json message;
    four_bars = four_bars + 1;
//    SLEEP(500)
  }
}
void send_start_message(int clock_rate, session_room *room, long midi_buffer) {
  nlohmann::json message;
  message["bytes"][0] = MIDI_CMD_COMMON_START;
  message["meta"]["uuid"] = room->GetUuid();
  message["meta"]["clock_rate"] = clock_rate;
  message["meta"]["exec_timestamp"] = get_posix_timestamp(midi_buffer);
  LOG(DEBUG) << "MIDI start: " << message.dump();
  room->deliver(message);
}

awaitable<void> listener(tcp::acceptor acceptor) {
  session_room room;
  server_config server_config;
  std::thread midi_clock_thread(midi_clock,
                                server_config.tick_interval,
                                server_config.loop_length,
                                &room,
                                server_config.midi_buffer);
  for (;;) {
    const std::shared_ptr<client_session> &session = std::make_shared<client_session>(
        co_await acceptor.async_accept(use_awaitable),
        room
    );
    session->start();
    LOG(INFO) << "Client session started\n";
  }
}

int main(int argc, char *argv[]) {

  try {
    if (argc < 2) {
      LOG(DEBUG) << "Usage: chat_server <port> [<port> ...]\n";
      return 1;
    }
    const thread &server_thread = audio_server.spawn();
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
