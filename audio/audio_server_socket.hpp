/*
 * my5t3ry wuuuuh :)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sox.h>
#include <roc/config.h>
#include <roc/context.h>
#include <roc/address.h>
#include <roc/log.h>
#include <roc/sender.h>
#include <roc/frame.h>
#include <roc/receiver.h>
#include <audio/signal-estimator/src/Config.hpp>

#include "utils/log.hpp"

class audio_server_socket {
 public:
  std::thread spawn() {
    return std::thread(&audio_server_socket::init_audio_socket, this);
  }

  void add_client_transmission(string client_ip) {
    signal_estimator::Config config;
    LOG(INFO) << "connecting audi socket at: " << client_ip;
    roc_log_set_level(ROC_LOG_DEBUG);
    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));

    roc_context *context = roc_context_open(&context_config);
    if (!context) {
      LOG(ERROR) << "roc_context_open";
    }

    roc_sender_config sender_config;
    memset(&sender_config, 0, sizeof(sender_config));
    sender_config.frame_sample_rate = config.sample_rate;

    sender_config.frame_channels = ROC_CHANNEL_SET_STEREO;
    sender_config.frame_encoding = ROC_FRAME_ENCODING_PCM_FLOAT;
    sender_config.resampler_profile = ROC_RESAMPLER_DISABLE;
    sender_config.automatic_timing = 1;

    roc_sender *sender = roc_sender_open(context, &sender_config);
    if (!sender) {
      LOG(ERROR) << "roc_sender_open";
    }
    roc_address sender_addr;
    if (roc_address_init(&sender_addr, ROC_AF_AUTO, client_ip.c_str(), 5500) != 0) {
      LOG(ERROR) << "roc_address_init";
    }
    if (roc_sender_bind(sender, &sender_addr) != 0) {
      LOG(ERROR) << "roc_sender_bind";
    }
    roc_address recv_source_addr;
    if (roc_address_init(&recv_source_addr, ROC_AF_AUTO, client_ip.c_str(), 2000) != 0) {
      LOG(ERROR) << "roc_address_init";
    }
    if (roc_sender_connect(sender, ROC_PORT_AUDIO_SOURCE, ROC_PROTO_RTP_RS8M_SOURCE,
                           &recv_source_addr) != 0) {
      LOG(ERROR) << "roc_sender_connect";
    }

    roc_address recv_repair_addr;
    if (roc_address_init(&recv_repair_addr, ROC_AF_AUTO, client_ip.c_str(),
                         2001) != 0) {
      LOG(ERROR) << "roc_address_init";
    }
    if (roc_sender_connect(sender, ROC_PORT_AUDIO_REPAIR, ROC_PROTO_RS8M_REPAIR,
                           &recv_repair_addr) != 0) {
      LOG(ERROR) << "roc_sender_connect";
    }
    senders.push_back(sender);
  }

 public:
  void init_audio_socket() {
    roc_log_set_level(ROC_LOG_DEBUG);
    signal_estimator::Config config;
    server_config server_config;

    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));

    roc_context *context = roc_context_open(&context_config);
    if (!context) {
      LOG(DEBUG) << "roc_context_open";
    }

    roc_receiver_config receiver_config;
    memset(&receiver_config, 0, sizeof(receiver_config));
    receiver_config.frame_sample_rate = config.sample_rate;
    receiver_config.frame_channels = ROC_CHANNEL_SET_STEREO;
    receiver_config.frame_encoding = ROC_FRAME_ENCODING_PCM_FLOAT;
    receiver_config.resampler_profile = ROC_RESAMPLER_DISABLE;
//    receiver_config.max_latency_overrun =  500000LL;
//    receiver_config.max_latency_underrun = 20000LL;
    receiver_config.target_latency = 6000000000LL;
    receiver_config.automatic_timing = 1;
    roc_receiver *receiver = roc_receiver_open(context, &receiver_config);
    if (!receiver) {
      LOG(DEBUG) << "roc_receiver_open";
    }
    roc_address recv_source_addr;
    if (roc_address_init(&recv_source_addr, ROC_AF_AUTO, server_config.bind_address.c_str(),
                         server_config.audio_data_port) != 0) {
      LOG(DEBUG) << "roc_address_init";
    }
    if (roc_receiver_bind(receiver, ROC_PORT_AUDIO_SOURCE, ROC_PROTO_RTP_RS8M_SOURCE,
                          &recv_source_addr) != 0) {
      LOG(DEBUG) << "roc_receiver_bind";
    }

    roc_address recv_repair_addr;
    if (roc_address_init(&recv_repair_addr, ROC_AF_AUTO, server_config.bind_address.c_str(),
                         server_config.audio_repair_port) != 0) {
      LOG(DEBUG) << "roc_address_init";
    }
    if (roc_receiver_bind(receiver, ROC_PORT_AUDIO_REPAIR, ROC_PROTO_RS8M_REPAIR,
                          &recv_repair_addr) != 0) {
      LOG(DEBUG) << "roc_receiver_bind";
    }

    for (;;) {
      float recv_samples[config.buffer_size];
      roc_frame frame;
      memset(&frame, 0, sizeof(frame));
      frame.samples = recv_samples;
      frame.samples_size = config.buffer_size * sizeof(float);
      if (roc_receiver_read(receiver, &frame) != 0) {
        break;
      } else {
        LOG(TRACE) << "forwarding frame with size: " << &frame.samples_size;
        if (!senders.empty()) {
          roc_sender_write(senders.data()[0], &frame);
        }
      }
    }

    if (roc_receiver_close(receiver) != 0) {
      LOG(ERROR) << "roc_receiver_close";
    }

    if (roc_context_close(context) != 0) {
      LOG(ERROR) << "roc_context_close";
    }
  }

 private:
  std::vector<roc_sender *> senders;
};
