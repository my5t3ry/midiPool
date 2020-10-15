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
#include <roc/frame.h>
#include <roc/receiver.h>

#include "utils/log.hpp"


/* Receiver parameters. */

#define EXAMPLE_SENDER_IP "0.0.0.0"
#define EXAMPLE_SENDER_PORT 0

/* Player parameters. */
#define EXAMPLE_OUTPUT_DEVICE "default"
#define EXAMPLE_OUTPUT_TYPE "alsa"
#define EXAMPLE_SAMPLE_RATE 44100
#define EXAMPLE_NUM_CHANNELS 2
#define EXAMPLE_BUFFER_SIZE 1000

#define EXAMPLE_CLIENT_RECEIVER_IP "0.0.0.0"
#define EXAMPLE_CLIENT_RECEIVER_SOURCE_PORT 20000
#define EXAMPLE_CLIENT_RECEIVER_REPAIR_PORT 20001

/* Signal parameters */
#define EXAMPLE_SAMPLE_RATE 44100
#define EXAMPLE_SINE_RATE 440
#define EXAMPLE_SINE_SAMPLES (EXAMPLE_SAMPLE_RATE * 5)
#define EXAMPLE_BUFFER_SIZE 100

class audio_transmitter {
 public:
  static void init_audio_transmitter(void *target_ip_ptr) {
    std::string target_ip = *reinterpret_cast<std::string *>(target_ip_ptr);

    LOG(INFO) << "connecting audi socket at: " << target_ip;
    connection_config connection_config;
    roc_context_config sender_context_config;
    memset(&sender_context_config, 0, sizeof(sender_context_config));

    /* Create sender_context.
     * Context contains memory pools and the network worker thread(s).
     * We need a sender_context to create a sender. */
    roc_context *sender_context = roc_context_open(&sender_context_config);
    if (!sender_context) {
      LOG(ERROR) << "roc_sendercontext_open";
    }

    /* Initialize sender config.
     * Initialize to zero to use default values for unset fields. */
    roc_sender_config sender_config;
    memset(&sender_config, 0, sizeof(sender_config));

    /* Setup input frame format. */
    sender_config.frame_sample_rate = EXAMPLE_SAMPLE_RATE;
    sender_config.frame_channels = ROC_CHANNEL_SET_STEREO;
    sender_config.frame_encoding = ROC_FRAME_ENCODING_PCM_FLOAT;

    /* Turn on sender timing.
     * Sender must send packets with steady rate, so we should either implement
     * clocking or ask the library to do so. We choose the second here. */
    sender_config.automatic_timing = 1;

    /* Create sender. */
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

    /* Connect sender to the receiver source (audio) packets port.
     * The receiver should expect packets with RTP header and Reed-Solomon (m=8) FECFRAME
    to_addressayload ID on that port. */
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
    size_t i;
    for (;;) {
      /* Generate sine wave. */
      float samples[EXAMPLE_BUFFER_SIZE];
      audio_transmitter::gensine(samples, EXAMPLE_BUFFER_SIZE);

      /* Write samples to the sender. */
      roc_frame frame;
      memset(&frame, 0, sizeof(frame));

      frame.samples = samples;
      frame.samples_size = EXAMPLE_BUFFER_SIZE * sizeof(float);

      if (roc_sender_write(sender, &frame) != 0) {
        LOG(DEBUG) << "roc_sender_write";
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
  static void gensine(float *samples, size_t num_samples) {
    double t = 0;
    size_t i;
    for (i = 0; i < num_samples / 2; i++) {
      const float s =
          (float) sin(2 * 3.14159265359 * EXAMPLE_SINE_RATE / EXAMPLE_SAMPLE_RATE * t);

      /* Fill samples for left and right channels. */
      samples[i * 2] = s;
      samples[i * 2 + 1] = -s;

      t += 1;
    }
  }
};
