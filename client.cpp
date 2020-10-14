#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "common.hpp"
#include "midi_cue.hpp"
#include "chat_client.hpp"

enum {
  max_length = 1024
};
int main(int argc, char *argv[]) {
  try {
    if (argc != 3) {
      LOG(DEBUG) << "Usage: chat_client <host> <port>\n";
      return 1;
    }
    boost::asio::io_context io_context(1);
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[1], argv[2]);
    static chat_client c(io_context, endpoints);
    LOG(INFO) << "client connecting to: " << argv[1] << ":" << argv[2];
    midi_cue midi_cue;
    midi_cue::init(&c.GetUuid());
    std::thread send_midi_messages_thread(midi_cue::send_midi_messages);
    std::thread send_midi_clock_thread(midi_cue::send_clock);
    LOG(INFO) << "midi spooler threads initialized.";
    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) { io_context.stop(); });
    io_context.run();
  }
  catch (std::exception &e) {
    LOG(ERROR) << "Exception: " << e.what() << "\n";
  }
  return 0;
}

