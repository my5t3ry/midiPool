//
// Created by my5t3ry on 10/15/20.
//

#ifndef MIDIPOOL_UTILS_STRUCTS_HPP_
#define MIDIPOOL_UTILS_STRUCTS_HPP_

typedef float MY_TYPE;
#define FORMAT RTAUDIO_FLOAT32

struct midi_message {
  std::vector<unsigned char> *message_bytes;
  long int timestamp;
  int clock_rate;
};

struct connection_config {
  string target_ip;
  int sender_port = 1050;
  int data_port = 1000;
  int repair_port = 1001;
};
struct InputData {
  MY_TYPE* buffer;
  unsigned long bufferBytes;
  unsigned long totalFrames;
  unsigned long frameCounter;
  unsigned int channels;
};

enum {
  MaxBufSize = 1000,

  SampleRate = 44100,
  ChMask = 0x3,
  NumCh = 2,

  SamplesPerFrame = 20,
  SamplesPerPacket = 100,
  FramesPerPacket = SamplesPerPacket / SamplesPerFrame,

  ManyFrames = FramesPerPacket * 20
};

#endif //MIDIPOOL_UTILS_STRUCTS_HPP_
