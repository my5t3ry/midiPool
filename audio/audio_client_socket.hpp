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
#include <audio/signal-estimator/src/AlsaWriter.hpp>

#include "utils/log.hpp"

class audio_client_socket {
 public:
  static void init_audio_socket(int audio_data_port_ = 1000, int audio_repair_port_ = 10001) {
    roc_log_set_level(ROC_LOG_DEBUG);
    signal_estimator::Config config;
    std::string bind_address = "127.0.0.1";
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
    if (roc_address_init(&recv_source_addr, ROC_AF_AUTO, bind_address.c_str(),
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
    if (roc_address_init(&recv_repair_addr, ROC_AF_AUTO, bind_address.c_str(),
                         audio_repair_port_)
        != 0) {
      LOG(ERROR) << "audio socket receiver roc_address_init failed";
    }
    if (roc_receiver_bind(receiver, ROC_PORT_AUDIO_REPAIR, ROC_PROTO_RS8M_REPAIR,
                          &recv_repair_addr)
        != 0) {
      LOG(ERROR) << "audio socket receiver roc_receiver_bind failed";
    }
    LOG(INFO) << "audio socket is listening on: " << bind_address << "/"
              << audio_data_port_ << ":" << audio_repair_port_;

    roc_context_config sender_context_config;
    memset(&sender_context_config, 0, sizeof(sender_context_config));
    roc_context *sender_context = roc_context_open(&sender_context_config);
    if (!sender_context) {
      LOG(ERROR) << "roc_sendercontext_open";
    }
    std::string device = "hw,0:0";
    signal_estimator::AlsaWriter alsa_writer;
    /* Receive and play samples. */
    for (;;) {
      /* Read samples from receiver.
       * If not enough samples are received, receiver will pad buffer with zeros. */
      float recv_samples[config.buffer_size];

      roc_frame frame;
      memset(&frame, 0, sizeof(frame));

      frame.samples = recv_samples;
      frame.samples_size = frame.samples_size * sizeof(float);

      if (roc_receiver_read(receiver, &frame) != 0) {
        break;
      } else {
        signal_estimator::Frame out_frame(config);
        ssize_t n;
        for (n = 0; n < config.buffer_size; n++) {
          float sampleFloat = recv_samples[n];
          sampleFloat *= 32767;
          int16_t sampleInt = (int16_t) sampleFloat;
          out_frame.add_data(sampleInt);
        }
        alsa_writer.write(out_frame);
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
};
