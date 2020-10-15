//
// Created by my5t3ry on 10/14/20.
//

#ifndef MIDIPOOL__SERVER_CONFIG_HPP_
#define MIDIPOOL__SERVER_CONFIG_HPP_

class server_config {
 public:
  const int loop_length = 8;
  const int tick_interval = 25;
  const int midi_buffer = 30;
  const string bind_address = "127.0.0.1";
  const int audio_data_port = 1000;
  const int audio_repair_port = 1001;
};

#endif //MIDIPOOL__SERVER_CONFIG_HPP_
