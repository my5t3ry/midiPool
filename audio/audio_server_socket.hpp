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
  audio_server_socket(int audio_data_port, int audio_repair_port)
      : audio_data_port_(audio_data_port), audio_repair_port_(audio_repair_port) {}
 public:
  void SetSenders(roc_sender *sender) {
    senders.push_back(sender);
  }
  std::thread spawn() {
    return std::thread(&audio_server_socket::init_audio_socket, this);
  }
 public:
  void init_audio_socket() {
    roc_log_set_level(ROC_LOG_DEBUG);
    signal_estimator::Config config;
    server_config server_config;

    /* Initialize context config.
     * Initialize to zero to use default values for all fields. */
    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));

    /* Create context.
     * Context contains memory pools and the network worker thread(s).
     * We need a context to create a receiver. */
    roc_context *context = roc_context_open(&context_config);
    if (!context) {
      LOG(ERROR) << "audio socket receiver roc_context_open failed";
    }

    /* Initialize receiver config.
     * We use default values. */
    roc_receiver_config receiver_config;
    memset(&receiver_config, 0, sizeof(receiver_config));

    /* Setup output frame format. */
    receiver_config.frame_sample_rate = config.sample_rate;
    receiver_config.frame_channels = ROC_CHANNEL_SET_STEREO;
    receiver_config.frame_encoding = ROC_FRAME_ENCODING_PCM_FLOAT;

    /* Create receiver. */
    roc_receiver *receiver = roc_receiver_open(context, &receiver_config);
    if (!receiver) {
      LOG(ERROR) << "audio socket receiver roc_receiver_open failed";
    }

    /* Bind receiver to the source (audio) packets port.
     * The receiver will expect packets with RTP header and Reed-Solomon (m=8) FECFRAME
     * Source Payload ID on this port. */
    roc_address recv_source_addr;
    if (roc_address_init(&recv_source_addr, ROC_AF_AUTO, server_config.bind_address.c_str(),
                         audio_data_port_)
        != 0) {
      LOG(ERROR) << "audio socket receiver roc_address_init failed";
    }

    if (roc_receiver_bind(receiver, ROC_PORT_AUDIO_SOURCE, ROC_PROTO_RTP_RS8M_SOURCE,
                          &recv_source_addr)
        != 0) {
      LOG(ERROR) << "audio socket receiver roc_receiver_bind failed";
    }

    /* Bind receiver to the repair (FEC) packets port.
     * The receiver will expect packets with Reed-Solomon (m=8) FECFRAME
     * Repair Payload ID on this port. */
    roc_address recv_repair_addr;
    if (roc_address_init(&recv_repair_addr, ROC_AF_AUTO, server_config.bind_address.c_str(),
                         audio_repair_port_)
        != 0) {
      LOG(ERROR) << "audio socket receiver roc_address_init failed";
    }
    if (roc_receiver_bind(receiver, ROC_PORT_AUDIO_REPAIR, ROC_PROTO_RS8M_REPAIR,
                          &recv_repair_addr)
        != 0) {
      LOG(ERROR) << "audio socket receiver roc_receiver_bind failed";
    }
    LOG(INFO) << "audio socket is listening on: " << server_config.bind_address << "/"
              << audio_data_port_ << ":" << audio_repair_port_;



    /* Receive and play samples. */
    for (;;) {
      /* Read samples from receiver.
       * If not enough samples are received, receiver will pad buffer with zeros. */
      float recv_samples[config.buffer_size];

      roc_frame frame;
      memset(&frame, 0, sizeof(frame));
      frame.samples = recv_samples;
      frame.samples_size = config.buffer_size * sizeof(float);
      if (roc_receiver_read(receiver, &frame) != 0) {
        break;
      } else {
//        LOG(DEBUG) << "forwarding frame with size: " << &frame.samples_size;
        if (!senders.empty()) {
          roc_sender_write(senders.data()[0], &frame);
        }
      }
    }

    /* Destroy receiver. */
    if (roc_receiver_close(receiver) != 0) {
      LOG(ERROR) << "roc_receiver_close";
    }

    /* Destroy context. */
    if (roc_context_close(context) != 0) {
      LOG(ERROR) << "roc_context_close";
    }
  }

 private:
  int audio_data_port_;
  int audio_repair_port_;
  std::vector<roc_sender *> senders;

};
