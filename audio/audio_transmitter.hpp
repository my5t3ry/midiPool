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

#include <soundio/soundio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>

struct SoundIoRingBuffer *ring_buffer = NULL;

static enum SoundIoFormat prioritized_formats[] = {
    SoundIoFormatFloat32NE,
    SoundIoFormatFloat32FE,
    SoundIoFormatS32NE,
    SoundIoFormatS32FE,
    SoundIoFormatS24NE,
    SoundIoFormatS24FE,
    SoundIoFormatS16NE,
    SoundIoFormatS16FE,
    SoundIoFormatFloat64NE,
    SoundIoFormatFloat64FE,
    SoundIoFormatU32NE,
    SoundIoFormatU32FE,
    SoundIoFormatU24NE,
    SoundIoFormatU24FE,
    SoundIoFormatU16NE,
    SoundIoFormatU16FE,
    SoundIoFormatS8,
    SoundIoFormatU8,
    SoundIoFormatInvalid,
};
static int prioritized_sample_rates[] = {
    48000,
    44100,
    96000,
    24000,
    0,
};

class audio_transmitter {
 public:
  static int min_int(int a, int b) {
    return (a < b) ? a : b;
  }
  static void overflow_callback(struct SoundIoInStream *instream) {
    static int count = 0;
    LOG(ERROR) << "overflow " << ++count;
  }
  static void read_callback(struct SoundIoInStream *instream, int frame_count_min, int frame_count_max) {
    struct SoundIoChannelArea *areas;
    int err;
    char *write_ptr = soundio_ring_buffer_write_ptr(ring_buffer);
    int free_bytes = soundio_ring_buffer_free_count(ring_buffer);
    int free_count = free_bytes / instream->bytes_per_frame;
    if (frame_count_min > free_count)
      LOG(ERROR) << "ring buffer overflow";
    int write_frames = min_int(free_count, frame_count_max);
    int frames_left = write_frames;
    for (;;) {
      int frame_count = frames_left;
      if ((err = soundio_instream_begin_read(instream, &areas, &frame_count)))
        LOG(ERROR) << "begin read error: " << soundio_strerror(err);
      if (!frame_count)
        break;
      if (!areas) {
        // Due to an overflow there is a hole. Fill the ring buffer with
        // silence for the size of the hole.
        memset(write_ptr, 0, frame_count * instream->bytes_per_frame);
        LOG(ERROR) << "Dropped " << frame_count << " frames due to internal overflow";
      } else {
        for (int frame = 0; frame < frame_count; frame += 1) {
          for (int ch = 0; ch < instream->layout.channel_count; ch += 1) {
            memcpy(write_ptr, areas[ch].ptr, instream->bytes_per_sample);
            areas[ch].ptr += areas[ch].step;
            write_ptr += instream->bytes_per_sample;
          }
        }
      }
      if ((err = soundio_instream_end_read(instream)))
        LOG(ERROR) << "end read error: " << soundio_strerror(err);
      frames_left -= frame_count;
      if (frames_left <= 0)
        break;
    }
    int advance_bytes = write_frames * instream->bytes_per_frame;
    soundio_ring_buffer_advance_write_ptr(ring_buffer, advance_bytes);
  }

  static void initInputStreamReader(roc_sender *p_sender) {
    enum SoundIoBackend backend = SoundIoBackendNone;
    char *device_id = NULL;
    bool is_raw = false;
    backend = SoundIoBackendAlsa;

    struct SoundIo *soundio = soundio_create();
    if (!soundio) {
      LOG(ERROR) << "out of memory\n";
    }
    int err = (backend == SoundIoBackendNone) ?
              soundio_connect(soundio) : soundio_connect_backend(soundio, backend);
    if (err) {
      LOG(ERROR) << "error connecting: " << soundio_strerror(err);
    }
    soundio_flush_events(soundio);
    int default_in_device_index = soundio_default_input_device_index(soundio);
    if (default_in_device_index < 0)
      LOG(ERROR) << "no input device found";

    struct SoundIoDevice *selected_device = NULL;
    if (default_in_device_index >= 0) {
      for (int i = 0; i < soundio_input_device_count(soundio); i += 1) {
        struct SoundIoDevice *device = soundio_get_input_device(soundio, i);

        if (device->is_raw == is_raw) {
          selected_device = device;
          break;
        }
        soundio_device_unref(device);
      }
      if (!selected_device) {
        LOG(ERROR) << "Invalid device id: " << default_in_device_index;
      }
    } else {
      LOG(ERROR) << "No input devices available.";
    }
    LOG(ERROR) << "Device: " << selected_device->name;
    if (selected_device->probe_error) {
      LOG(ERROR) << "Unable to probe device: " << soundio_strerror(selected_device->probe_error);
    }
    soundio_device_sort_channel_layouts(selected_device);
    int sample_rate = 0;
    int *sample_rate_ptr;
    for (sample_rate_ptr = prioritized_sample_rates; *sample_rate_ptr; sample_rate_ptr += 1) {
      if (soundio_device_supports_sample_rate(selected_device, *sample_rate_ptr)) {
        sample_rate = *sample_rate_ptr;
        break;
      }
    }
    if (!sample_rate)
      sample_rate = selected_device->sample_rates[0].max;
    enum SoundIoFormat fmt = SoundIoFormatInvalid;
    enum SoundIoFormat *fmt_ptr;
    for (fmt_ptr = prioritized_formats; *fmt_ptr != SoundIoFormatInvalid; fmt_ptr += 1) {
      if (soundio_device_supports_format(selected_device, *fmt_ptr)) {
        fmt = *fmt_ptr;
        break;
      }
    }
    if (fmt == SoundIoFormatInvalid)
      fmt = selected_device->formats[0];

    struct SoundIoInStream *instream = soundio_instream_create(selected_device);
    if (!instream) {
      LOG(ERROR) << "out of memory";
    }
    instream->format = fmt;
    instream->sample_rate = sample_rate;
    instream->read_callback = read_callback;
    instream->overflow_callback = overflow_callback;
    instream->userdata = &ring_buffer;
    if ((err = soundio_instream_open(instream))) {
      LOG(ERROR) << "unable to open input stream: " << soundio_strerror(err);
    }
    LOG(ERROR) <<  instream->layout.name <<" "<< sample_rate <<" "<< soundio_format_string(fmt)<<" interleaved";
    const int ring_buffer_duration_seconds = 30;
    int capacity = ring_buffer_duration_seconds * instream->sample_rate * instream->bytes_per_frame;
    ring_buffer = soundio_ring_buffer_create(soundio, capacity);
    if (!ring_buffer)
      LOG(ERROR) << "unable to create ring buffer: out of memory";

    if ((err = soundio_instream_start(instream))) {
      LOG(ERROR) << "unable to start input device: " << soundio_strerror(err);
    }
    // Note: in this example, if you send SIGINT (by pressing Ctrl+C for example)
    // you will lose up to 1 second of recorded audio data. In non-example code,
    // consider a better shutdown strategy.
    for (;;) {
      soundio_flush_events(soundio);
      sleep(1);
      int fill_bytes = soundio_ring_buffer_fill_count(ring_buffer);
      char *read_buf = soundio_ring_buffer_read_ptr(ring_buffer);
      roc_frame frame;
      memset(&frame, 0, sizeof(frame));
      frame.samples = read_buf;
      frame.samples_size = fill_bytes;
      if (roc_sender_write(p_sender, &frame) != 0) {
        LOG(ERROR) << "write error: " << strerror(errno);
        break;
      }
      soundio_ring_buffer_advance_read_ptr(ring_buffer, fill_bytes);
    }
  }
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

//    std::string device = "plughw:PCH,0:0";
//    config.format = SND_PCM_FORMAT_FLOAT;
//    signal_estimator::AlsaReader alsa_reader;
//    alsa_reader.open(config, device.c_str());
    /* Open SoX output device. */
    /* Receive and play samples. */
    initInputStreamReader(sender);

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
