//
// Created by my5t3ry on 10/14/20.
//

#ifndef MIDIPOOL__SERVER_CONFIG_HPP_
#define MIDIPOOL__SERVER_CONFIG_HPP_

class server_config {
 public:
  int GetLoopLength() const {
    return loop_length;
  }
  int GetTickInterval() const {
    return tick_interval;
  }
  int GetMidiBuffer() const {
    return midi_buffer;
  }
 private:
  int loop_length = 0;
  int tick_interval = 25;
  int midi_buffer = 30;
};
#endif //MIDIPOOL__SERVER_CONFIG_HPP_
