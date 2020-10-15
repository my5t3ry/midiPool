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
  static void init_audio_socket(const int audio_data_port_ = 1000, const int audio_repair_port_ = 1001) {
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

    roc_context_config sender_context_config;
    memset(&sender_context_config, 0, sizeof(sender_context_config));
    roc_context *sender_context = roc_context_open(&sender_context_config);
    if (!sender_context) {
      LOG(ERROR) << "roc_sendercontext_open";
    }

    roc_sender_config sender_config;
    memset(&sender_config, 0, sizeof(sender_config));

    std::string target_ip = "127.0.0.1";
    sender_config.frame_sample_rate = config.sample_rate;
    sender_config.frame_channels = ROC_CHANNEL_SET_STEREO;
    sender_config.frame_encoding = ROC_FRAME_ENCODING_PCM_FLOAT;
    sender_config.automatic_timing = 1;
    roc_sender *sender = roc_sender_open(sender_context, &sender_config);
    if (!sender) {
      LOG(ERROR) << "roc_sender_open";
    }
    /* Bind sender to a random port. */
    roc_address sender_addr;
    if (roc_address_init(&sender_addr, ROC_AF_AUTO, target_ip.c_str(),
                         5000)
        != 0) {
      LOG(ERROR) << "roc_senderaddress_init";
    }
    if (roc_sender_bind(sender, &sender_addr) != 0) {
      LOG(ERROR) << "roc_sendersender_bind";
    }
    roc_address client_recv_source_addr;
    if (roc_address_init(&client_recv_source_addr, ROC_AF_AUTO, target_ip.c_str(),
                         2000)
        != 0) {
      LOG(ERROR) << "roc_address_init";
    }

    if (roc_sender_connect(sender, ROC_PORT_AUDIO_SOURCE, ROC_PROTO_RTP_RS8M_SOURCE,
                           &client_recv_source_addr)
        != 0) {
      LOG(ERROR) << "roc_sender_connect";
    }

    /* Connect sender to the receiver repair (FEC) packets port.
     * The receiver should expect packets with Reed-Solomon (m=8) FECFRAME
     * Repair Payload ID on that port. */
    roc_address client_recv_repair_addr;
    if (roc_address_init(&client_recv_repair_addr, ROC_AF_AUTO, target_ip.c_str(),
                         2001)
        != 0) {
      LOG(ERROR) << "roc_address_init";
    }
    if (roc_sender_connect(sender, ROC_PORT_AUDIO_REPAIR, ROC_PROTO_RS8M_REPAIR,
                           &client_recv_repair_addr)
        != 0) {
      LOG(ERROR) << "roc_sender_connect";
    }


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
        roc_sender_write(sender, &frame);
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
