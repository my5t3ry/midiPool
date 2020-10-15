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
#include <roc/sender.h>
#include <roc/log.h>
#include <roc/frame.h>
#include <roc/receiver.h>
#include <audio/signal-estimator/src/Config.hpp>
#include <audio/signal-estimator/src/AlsaReader.hpp>
#include "utils/log.hpp"

class audio_transmitter {
 public:
  static void init_audio_transmitter(void *target_ip_ptr) {
    std::string target_ip = *reinterpret_cast<std::string *>(target_ip_ptr);
    roc_log_set_level(ROC_LOG_DEBUG);

    LOG(INFO) << "connecting audi socket at: " << target_ip;
    connection_config connection_config;
    roc_context_config sender_context_config;
    memset(&sender_context_config, 0, sizeof(sender_context_config));
    roc_context *sender_context = roc_context_open(&sender_context_config);
    if (!sender_context) {
      LOG(ERROR) << "roc_sendercontext_open";
    }

    roc_sender_config sender_config;
    memset(&sender_config, 0, sizeof(sender_config));

    signal_estimator::Config config;

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
                         connection_config.sender_port)
        != 0) {
      LOG(ERROR) << "roc_senderaddress_init";
    }
    if (roc_sender_bind(sender, &sender_addr) != 0) {
      LOG(ERROR) << "roc_sendersender_bind";
    }
    roc_address client_recv_source_addr;
    if (roc_address_init(&client_recv_source_addr, ROC_AF_AUTO, target_ip.c_str(),
                         connection_config.data_port)
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
                         connection_config.repair_port)
        != 0) {
      LOG(ERROR) << "roc_address_init";
    }
    if (roc_sender_connect(sender, ROC_PORT_AUDIO_REPAIR, ROC_PROTO_RS8M_REPAIR,
                           &client_recv_repair_addr)
        != 0) {
      LOG(ERROR) << "roc_sender_connect";
    }

    std::string device = "hw:0,0";
    signal_estimator::AlsaReader alsa_reader;
    alsa_reader.open(config, device.c_str());
    /* Open SoX output device. */
    /* Receive and play samples. */
    for (;;) {
      /* Read samples from receiver.
       * If not enough samples are received, receiver will pad buffer with zeros. */
      signal_estimator::Frame read_frame(config);
      for (size_t n = 0; n < config.total_samples() / read_frame.size(); n++) {
        if (!alsa_reader.read(read_frame)) {
          exit(1);
        }
      }

      roc_frame frame;
      memset(&frame, 0, sizeof(frame));
      frame.samples = read_frame.data();
      frame.samples_size = read_frame.size();

      if (roc_sender_write(sender, &frame) != 0) {
        break;
      }
    }

    /* Destroy sender. */
    if (roc_sender_close(sender) != 0) {
      LOG(ERROR) << "roc_sender_close";
    }

    /* Destroy context. */
    if (roc_context_close(sender_context) != 0) {
      LOG(ERROR) << "roc_context_close";
    }

  }

};
