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

static roc_sender* init_sender(string client_ip) {
  roc_log_set_level(ROC_LOG_DEBUG);
  signal_estimator::Config config;

  roc_context_config sender_context_config;
  memset(&sender_context_config, 0, sizeof(sender_context_config));
  roc_context *sender_context = roc_context_open(&sender_context_config);
  if (!sender_context) {
    LOG(ERROR) << "roc_sendercontext_open";
  }
  std::string target_ip = "127.0.0.1";

  roc_sender_config sender_config;
  memset(&sender_config, 0, sizeof(sender_config));

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
  if (roc_address_init(&client_recv_source_addr, ROC_AF_AUTO, client_ip.c_str(),
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
  if (roc_address_init(&client_recv_repair_addr, ROC_AF_AUTO, client_ip.c_str(),
                       2001)
      != 0) {
    LOG(ERROR) << "roc_address_init";
  }
  if (roc_sender_connect(sender, ROC_PORT_AUDIO_REPAIR, ROC_PROTO_RS8M_REPAIR,
                         &client_recv_repair_addr)
      != 0) {
    LOG(ERROR) << "roc_sender_connect";
  }
  return sender;
}
