#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "common.hpp"

enum {
  max_length = 1024
};

using boost::asio::ip::tcp;

using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::redirect_error;
using boost::asio::use_awaitable;
typedef std::deque<nlohmann::json> midi_message_queue;
#if defined(WIN32)
#include <windows.h>
#define SLEEP( milliseconds ) Sleep( (DWORD) milliseconds )
#else // Unix variants
#include <unistd.h>
#define SLEEP(milliseconds) usleep( (unsigned long) (milliseconds * 1000.0) )
#endif

class chat_client :
    public std::enable_shared_from_this<chat_client> {
 public:
  chat_client(boost::asio::io_context &io_context,
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
    write_msgs_.push_back(msg);
    if (writing != 1) {
      writer();

    }
  }

  void do_connect(const tcp::resolver::results_type &endpoints) {
    boost::asio::async_connect(socket_, endpoints,
                               [this](boost::system::error_code ec, tcp::endpoint) {
                                 if (!ec) {
                                   co_spawn(socket_.get_executor(),
                                            [this] { return this->reader(); },
                                            detached);
                                 }
                               });
  }

  awaitable<void> reader() {
    while (socket_.is_open()) {
      try {
        std::string read_msg;
        std::size_t n = co_await boost::asio::async_read_until(socket_,
                                                               boost::asio::dynamic_buffer(read_msg, 2048),
                                                               "\n",
                                                               use_awaitable);
        if (n > 0) {
          nlohmann::json cur_message = nlohmann::json::parse(read_msg.substr(0, n));
          LOG(DEBUG) << cur_message.dump();
          std::vector<unsigned char> message;
          message.clear();
          message.push_back(cur_message["bytes"][0]);
          midiout->sendMessage(&message);
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
      while (!write_msgs_.empty()) {
        co_await boost::asio::async_write(socket_,
                                          boost::asio::buffer(write_msgs_.front().dump()), use_awaitable);
        co_await boost::asio::async_write(socket_,
                                          boost::asio::buffer("\n"), use_awaitable);
        write_msgs_.pop_front();
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
  boost::asio::steady_timer timer_;
  std::string uuid;
  int writing = 0;
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
      LOG(DEBUG) << "Usage: chat_client <host> <port>\n";
      return 1;
    }

    boost::asio::io_context io_context(1);
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[1], argv[2]);
    const std::shared_ptr<chat_client> &chat_session = std::make_shared<chat_client>(
        io_context,
        endpoints
    );
    static chat_client c(io_context, endpoints);
    chat_session->init_midi();
    chat_session->init_callback(&midi_callback, &c);
    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) { io_context.stop(); });
    io_context.run();
  }
  catch (std::exception &e) {
    LOG(ERROR) << "Exception: " << e.what() << "\n";
  }
  return 0;
}
