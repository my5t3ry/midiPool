//
// Created by my5t3ry on 10/14/20.
//

#ifndef MIDIPOOL__SERVER_CONFIG_HPP_
#define MIDIPOOL__SERVER_CONFIG_HPP_

class server_config {
 public:
  const int GetLoopLength() {
    return loop_length;
  }
  const  int GetTickInterval() {
    return tick_interval;
  }
  const int GetMidiBuffer() {
    return midi_buffer;
  }
  const string &GetBindAddress() {
    return bind_address;
  }
  const int GetDataPort() {
    return data_port;
  }
  const int GetRepairPort() {
    return repair_port;
  }
 private:
  const int loop_length = 8;
  const int tick_interval = 25;
  const  int midi_buffer = 30;
  const string bind_address = "0.0.0.0";
  const int data_port = 1000;
  const int repair_port = 1001;
};
#endif //MIDIPOOL__SERVER_CONFIG_HPP_
