//
// Created by my5t3ry on 10/13/20.
//

#ifndef MIDIPOOL__MIDI_CUE_HPP_
#define MIDIPOOL__MIDI_CUE_HPP_

#include "common.hpp"

class midi_cue {
 public:
   static const midi_message_queue write_msgs_;
   static const std::vector<midi_message> midi_messages_;
   static const RtMidiOut *midi_out_;
   static const RtMidiIn *midi_in_;
   static const int clock_rate = 0;
   static const const string *uuid_;

  static void init(const string *uuid) {
    uuid_ = uuid;
    LOG(DEBUG) << "init midi";
    midi_out_ = new RtMidiOut();
    midi_in_ = new RtMidiIn();
    LOG(DEBUG) << "midi interface initialized.";
    midi_in_->openVirtualPort("midiPool send");
    midi_in_->ignoreTypes(false, false, false);
    midi_out_->openVirtualPort("midiPool receiver");
    midi_in_->ignoreTypes(false, false, false);
    midi_in_->setCallback(midi_cue::midi_callback);
    LOG(DEBUG) << "virtual midi ports initialized.";
  }

  static void midi_callback(double deltatime, std::vector<unsigned char> *message, void *userData) {
    int nBytes = message->size();
    if (nBytes > 0) {
      nlohmann::json j;
      for (int i = 0; i < nBytes; i++) {
        j["bytes"][i] = (int) message->data()[i];
      }
      j["meta"]["timestamp"] = deltatime;
      j["meta"]["uuid"] = *uuid_;
//    client->write(j);
    }
  }

  static void write(const nlohmann::json &msg) {
    write_msgs_.push_back(msg);
//    if (writing != 1) {
//      writer();
//
//    }
  }

  static midi_message build_midi_message(vector<unsigned char> *bytes,
                                         long timestamp,
                                         int clock_rate = 0) {
    midi_message midi_message;
    midi_message.message_bytes = bytes;
    midi_message.timestamp = timestamp;
    midi_message.clock_rate = clock_rate;
    return midi_message;
  }
  static void send_clock() {
    while (true) {
      if (clock_rate != 0) {
        vector<unsigned char> message_bytes;
        message_bytes.clear();
        message_bytes.push_back(static_cast<unsigned char>(MIDI_CMD_COMMON_CLOCK));
        midi_out_->sendMessage(&message_bytes);
        SLEEP(clock_rate);
      } else {
        SLEEP(10);
      }
    }
  }
  static void send_midi_messages() {
    std::vector<int> indices_to_erase;
    while (true) {
      if (!indices_to_erase.empty() > 0) {
        indices_to_erase.clear();
      }
      long cur_timestamp = get_posix_timestamp();
      int k;
      for (k = 0; k < midi_messages_.size(); k++) {
        midi_message &cur_message = midi_messages_.data()[k];
        if (cur_message.timestamp <= cur_timestamp) {
          LOG(DEBUG) << "sending midi message: " << cur_message.message_bytes << " with timestamp: "
                     << cur_message.timestamp << " at: " << cur_timestamp;
          if (!cur_message.message_bytes->empty()) {
            if ((cur_message.message_bytes->data()[0] == MIDI_CMD_COMMON_CONTINUE && clock_rate != 0)
                || cur_message.message_bytes->data()[0] == MIDI_CMD_COMMON_START) {
              clock_rate = cur_message.clock_rate;
              midi_out_->sendMessage(cur_message.message_bytes);
            }
            if (cur_message.message_bytes->data()[0] == MIDI_CMD_COMMON_STOP) {
              clock_rate = 0;
              midi_out_->sendMessage(cur_message.message_bytes);
            }
          }
          indices_to_erase.push_back(k);
        }
      }
      for (int cur_index : indices_to_erase) {
        midi_messages_.erase(midi_messages_.begin() + cur_index);
      }
      SLEEP(1);
    }
  }
  static void cue_midi_message(const midi_message &midi_message) {
    midi_messages_.push_back(midi_message);
  }
 private:

};

#endif //MIDIPOOL__MIDI_CUE_HPP_
