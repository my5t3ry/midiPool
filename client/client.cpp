#include <boost/uuid/uuid_io.hpp>
#include <audio/audio_client_socket.hpp>
#include "utils/common.hpp"
#include "midi/midi_cue.hpp"
#include "client_connection.hpp"

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
    static client_connection c(io_context, endpoints);
    std::make_shared<client_connection>(
        io_context, endpoints
    );
    LOG(INFO) << "client connecting to: " << endpoints->host_name() << ":" << argv[2];
    midi_cue midi_cue;
    midi_cue.init(const_cast<string &>(c.GetUuid()));
    string target_ip = boost::lexical_cast<std::string>(argv[1]);
    c.SetMidiCue(&midi_cue);
    std::thread send_midi_messages_thread(midi_cue::send_midi_messages, &midi_cue);
    std::thread send_midi_clock_thread(midi_cue::send_clock, &midi_cue);
    LOG(INFO) << "midi spooler threads initialized.";
    std::thread audio_transmitter
        (audio_transmitter::init_audio_transmitter, &target_ip);
    std::thread audio_socket
        (audio_client_socket::init_audio_socket, 2000, 2001);
    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) { io_context.stop(); });
    io_context.run();
  }
  catch (std::exception &e) {
    LOG(ERROR) << "Exception: " << e.what() << "\n";
  }
  return 0;
}

