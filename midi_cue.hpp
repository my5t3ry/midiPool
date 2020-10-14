//
// Created by my5t3ry on 10/13/20.
//

#ifndef MIDIPOOL__MIDI_CUE_HPP_
#define MIDIPOOL__MIDI_CUE_HPP_

#include "common.hpp"

class midi_cue {
 public:
  RtMidiOut *midi_out_;
  RtMidiIn *midi_in_;

  midi_message_queue write_msgs_;
  std::vector<midi_message> midi_messages_;
  int clock_rate = 0;
  string *uuid_;

  void init(string &uuid) {
    uuid_ = &uuid;
    LOG(DEBUG) << "init midi";
    clock_rate = 0;
    midi_out_ = new RtMidiOut();
    midi_in_ = new RtMidiIn();
    midi_in_->openVirtualPort("midiPool send");
    midi_in_->ignoreTypes(false, false, false);
    midi_out_->openVirtualPort("midiPool receiver");
    midi_in_->ignoreTypes(false, false, false);
    midi_in_->setCallback(midi_callback, this);
  }

  string *GetUuid() const {
    return uuid_;
  }
  static void midi_callback(double deltatime, std::vector<unsigned char> *message, void *userData) {
    int nBytes = message->size();
    midi_cue *kMidiCue = ((midi_cue *) userData);
    if (nBytes > 0) {
      nlohmann::json j;
      for (int i = 0; i < nBytes; i++) {
        j["bytes"][i] = (int) message->data()[i];
      }
      j["meta"]["timestamp"] = deltatime;
      j["meta"]["uuid"] = *kMidiCue->GetUuid();
//    client->write(j);
    }
  }

  void write(const nlohmann::json &msg) {
    write_msgs_.push_back(msg);
//    if (writing != 1) {
//      writer();
//
//    }
  }

  midi_message build_midi_message(vector<unsigned char> *bytes,
                                  long timestamp,
                                  int clock_rate = 0) {
    midi_message midi_message;
    midi_message.message_bytes = bytes;
    midi_message.timestamp = timestamp;
    midi_message.clock_rate = clock_rate;
    return midi_message;
  }
  static void send_clock(midi_cue *midi_cue) {
    while (true) {
      if (midi_cue->clock_rate != 0) {
        vector<unsigned char> message_bytes;
        message_bytes.clear();
        message_bytes.push_back(static_cast<unsigned char>(MIDI_CMD_COMMON_CLOCK));
        midi_cue->midi_out_->sendMessage(&message_bytes);
        SLEEP(midi_cue->clock_rate);
      } else {
        SLEEP(10);
      }
    }
  }
  static void send_midi_messages(midi_cue *midi_cue) {
    std::vector<int> indices_to_erase;
    while (true) {
      if (!indices_to_erase.empty() > 0) {
        indices_to_erase.clear();
      }
      long cur_timestamp = get_posix_timestamp();
      int k;
      for (k = 0; k < midi_cue->midi_messages_.size(); k++) {
        midi_message &cur_message = midi_cue->midi_messages_.data()[k];
        if (cur_message.timestamp >= cur_timestamp) {
          LOG(DEBUG) << "sending midi message: " << cur_message.message_bytes << " with timestamp: "
                     << cur_message.timestamp << " at: " << cur_timestamp;
          if (!cur_message.message_bytes->empty()) {
            if ((cur_message.message_bytes->data()[0] == MIDI_CMD_COMMON_CONTINUE && midi_cue->clock_rate != 0)
                || cur_message.message_bytes->data()[0] == MIDI_CMD_COMMON_START) {
              midi_cue->clock_rate = cur_message.clock_rate;
              midi_cue->midi_out_->sendMessage(cur_message.message_bytes);
            }
            if (cur_message.message_bytes->data()[0] == MIDI_CMD_COMMON_STOP
                || cur_message.message_bytes->data()[0] == MIDI_CMD_COMMON_SONG_POS) {
              midi_cue->midi_out_->sendMessage(cur_message.message_bytes);
              midi_cue->clock_rate = 0;
            }
            if (cur_message.message_bytes->data()[0] == MIDI_CMD_COMMON_SONG_POS) {
              midi_cue->clock_rate = 0;
              midi_message *stop_message;
              stop_message->message_bytes->push_back(MIDI_CMD_COMMON_STOP);
              midi_message *start_message;
              start_message->message_bytes->push_back(MIDI_CMD_COMMON_START);
              midi_cue->midi_out_->sendMessage(cur_message.message_bytes);
              SLEEP(1);
              midi_cue->midi_out_->sendMessage(stop_message.message_bytes);
              SLEEP(1);
              midi_cue->midi_out_->sendMessage(start_message.message_bytes);
              midi_cue->clock_rate = cur_message.clock_rate;
            }
          }
          indices_to_erase.push_back(k);
        }
      }
      for (int cur_index : indices_to_erase) {
        midi_cue->midi_messages_.erase(midi_cue->midi_messages_.begin() + cur_index);
      }
      SLEEP(1);
    }
  }
  void cue_midi_message(const midi_message &midi_message) {
    midi_messages_.push_back(midi_message);
  }
 private:

};

//
#endif //MIDIPOOL__MIDI_CUE_HPP_
