/*
 * my5t3ry wuuuuh :)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <soundio/soundio.h>

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

struct SoundIoRingBuffer *out_ring_buffer = NULL;

static enum SoundIoFormat out_prioritized_formats[] = {
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
static int out_prioritized_sample_rates[] = {
    48000,
    44100,
    96000,
    24000,
    0,
};

static void (*write_sample)(char *ptr, float sample);

class audio_client_socket {
 public:
  static int min_int(int a, int b) {
    return (a < b) ? a : b;
  }
  static void write_sample_s16ne(char *ptr, float sample) {
    int16_t *buf = (int16_t *) ptr;
    double range = (double) INT16_MAX - (double) INT16_MIN;
    double val = sample * range / 2.0;
    *buf = val;
  }
  static void write_sample_s32ne(char *ptr, float sample) {
    int32_t *buf = (int32_t *) ptr;
    double range = (double) INT32_MAX - (double) INT32_MIN;
    double val = sample * range / 2.0;
    *buf = val;
  }
  static void write_sample_float32ne(char *ptr, float sample) {
    float *buf = (float *) ptr;
    *buf = sample;
  }
  static void write_sample_float64ne(char *ptr, float sample) {
    double *buf = (double *) ptr;
    *buf = sample;
  }
  static void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
//    LOG(DEBUG) << "write_callback frame_count_min: " << frame_count_min << " frame_count_max: " << frame_count_max
//               << " ring buffer filled: " << soundio_ring_buffer_fill_count(out_ring_buffer);
    struct SoundIoChannelArea *areas;
    int frames_left;
    int frame_count;
    int err;
    char *read_ptr = soundio_ring_buffer_read_ptr(out_ring_buffer);
    int fill_bytes = soundio_ring_buffer_fill_count(out_ring_buffer);
    int fill_count = fill_bytes / outstream->bytes_per_frame;
    if (frame_count_min > fill_count) {
      // Ring buffer does not have enough data, fill with zeroes.
      frames_left = frame_count_min;
      for (;;) {
        frame_count = frames_left;
        if (frame_count <= 0)
          return;
        if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count)))
          LOG(DEBUG) << "begin write error: %s" << soundio_strerror(err);
        if (frame_count <= 0)
          return;
        for (int frame = 0; frame < frame_count; frame += 1) {
          for (int ch = 0; ch < outstream->layout.channel_count; ch += 1) {
            memset(areas[ch].ptr, 0, outstream->bytes_per_sample);
            areas[ch].ptr += areas[ch].step;
          }
        }
        if ((err = soundio_outstream_end_write(outstream)))
          LOG(DEBUG) << "end write error: %s" << soundio_strerror(err);
        frames_left -= frame_count;
      }
    }
    int read_count = min_int(frame_count_max, fill_count);
    frames_left = read_count;
    while (frames_left > 0) {
      int frame_count = frames_left;
      if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count)))
        LOG(DEBUG) << "begin write error: %s" << soundio_strerror(err);
      if (frame_count <= 0)
        break;
      for (int frame = 0; frame < frame_count; frame += 1) {
        for (int ch = 0; ch < outstream->layout.channel_count; ch += 1) {
          memcpy(areas[ch].ptr, read_ptr, outstream->bytes_per_sample);
          areas[ch].ptr += areas[ch].step;
          read_ptr += outstream->bytes_per_sample;
        }
      }
      if ((err = soundio_outstream_end_write(outstream)))
        LOG(DEBUG) << "end write error: %s" << soundio_strerror(err);
      frames_left -= frame_count;
    }
    soundio_ring_buffer_advance_read_ptr(out_ring_buffer, read_count * outstream->bytes_per_frame);
  }
  static void underflow_callback(struct SoundIoOutStream *outstream) {
    static int count = 0;
    LOG(ERROR) << "underflow %d" << count++;
  }

  static void initOutputStream(roc_receiver *p_receiver, signal_estimator::Config config) {
    char *stream_name = NULL;
    double latency = 0.0;
    enum SoundIoBackend backend = SoundIoBackendNone;
    char *device_id = nullptr;
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
    int default_in_device_index = soundio_default_output_device_index(soundio);
    if (default_in_device_index < 0)
      LOG(ERROR) << "no input device found";

    struct SoundIoDevice *selected_device = NULL;
    if (default_in_device_index >= 0) {
      for (int i = 0; i < soundio_input_device_count(soundio); i += 1) {
        struct SoundIoDevice *device = soundio_get_output_device(soundio, i);

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
      LOG(ERROR) << "No output devices available.";
    }
    LOG(ERROR) << "output Device: " << selected_device->name;
    if (selected_device->probe_error) {
      LOG(ERROR) << "Unable to probe device: " << soundio_strerror(selected_device->probe_error);
    }
    soundio_device_sort_channel_layouts(selected_device);
    int sample_rate = 0;
    int *sample_rate_ptr;
    for (sample_rate_ptr = out_prioritized_sample_rates; *sample_rate_ptr; sample_rate_ptr += 1) {
      if (soundio_device_supports_sample_rate(selected_device, *sample_rate_ptr)) {
        sample_rate = *sample_rate_ptr;
        break;
      }
    }
    if (!sample_rate)
      sample_rate = selected_device->sample_rates[0].max;
    enum SoundIoFormat fmt = SoundIoFormatInvalid;
    enum SoundIoFormat *fmt_ptr;
    for (fmt_ptr = out_prioritized_formats; *fmt_ptr != SoundIoFormatInvalid; fmt_ptr += 1) {
      if (soundio_device_supports_format(selected_device, *fmt_ptr)) {
        fmt = *fmt_ptr;
        break;
      }
    }
    if (fmt == SoundIoFormatInvalid)
      fmt = selected_device->formats[0];

    struct SoundIoOutStream *outstream = soundio_outstream_create(selected_device);
    if (!outstream) {
      LOG(ERROR) << "out of memory";
    }

    outstream->write_callback = write_callback;
    outstream->underflow_callback = underflow_callback;
    outstream->name = stream_name;
    outstream->software_latency = latency;
    outstream->sample_rate = sample_rate;
    if (soundio_device_supports_format(selected_device, SoundIoFormatFloat32NE)) {
      outstream->format = SoundIoFormatFloat32NE;
      write_sample = write_sample_float32ne;
    } else if (soundio_device_supports_format(selected_device, SoundIoFormatFloat64NE)) {
      outstream->format = SoundIoFormatFloat64NE;
      write_sample = write_sample_float64ne;
    } else if (soundio_device_supports_format(selected_device, SoundIoFormatS32NE)) {
      outstream->format = SoundIoFormatS32NE;
      write_sample = write_sample_s32ne;
    } else if (soundio_device_supports_format(selected_device, SoundIoFormatS16NE)) {
      outstream->format = SoundIoFormatS16NE;
      write_sample = write_sample_s16ne;
    } else {
      LOG(ERROR) << "No suitable selected_device format available.";
    }
    if ((err = soundio_outstream_open(outstream))) {
      LOG(ERROR) << "unable to open selected_device: " << soundio_strerror(err);
    }
    LOG(ERROR) << "Software latency: " << outstream->software_latency;

    if (outstream->layout_error)
      LOG(ERROR) << "unable to set channel layout: " << soundio_strerror(outstream->layout_error);
    if ((err = soundio_outstream_start(outstream))) {
      LOG(ERROR) << "unable to start device: " << soundio_strerror(err);
    }
    int capacity = outstream->software_latency * 2 * outstream->sample_rate * outstream->bytes_per_frame;
    out_ring_buffer = soundio_ring_buffer_create(soundio, capacity);
    if (!out_ring_buffer)
      LOG(ERROR) << "unable to create ring buffer: out of memory";
    char *buf = soundio_ring_buffer_write_ptr(out_ring_buffer);
    for (;;) {
      soundio_flush_events(soundio);
      LOG(DEBUG) << "flush_events";
      float recv_samples[config.buffer_size];
      roc_frame frame;
      memset(&frame, 0, sizeof(frame));
      frame.samples = recv_samples;
      frame.samples_size = config.buffer_size * sizeof(float);
      if (roc_receiver_read(p_receiver, &frame) != 0) {
        break;
      } else {
        memset(buf, 0, frame.samples_size);
        int free_bytes = soundio_ring_buffer_free_count(out_ring_buffer);
        int free_count = free_bytes / sizeof(float);
        LOG(DEBUG) << "free_count:" << free_count;
        LOG(DEBUG) << "frame.samples_size:" << frame.samples_size;
        if (frame.samples_size > free_count) {
          LOG(ERROR) << "ring buffer loop overflow";
          break;
        }
        for (int frameIndex = 0; frameIndex < frame.samples_size; frameIndex += 1) {
          write_sample(buf, recv_samples[frameIndex]);
          buf += sizeof(float);
        }
      }

      int advance_bytes = frame.samples_size;
      soundio_ring_buffer_advance_write_ptr(out_ring_buffer, advance_bytes);
    }
    soundio_outstream_destroy(outstream);
    soundio_device_unref(selected_device);
    soundio_destroy(soundio);
  }
  static void init_audio_socket(int audio_data_port_ = 1000, int audio_repair_port_ = 1001) {
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
    receiver_config.resampler_profile = ROC_RESAMPLER_DISABLE;
    receiver_config.frame_sample_rate = config.sample_rate;
    receiver_config.frame_channels = ROC_CHANNEL_SET_STEREO;
    receiver_config.frame_encoding = ROC_FRAME_ENCODING_PCM_FLOAT;
    receiver_config.automatic_timing = 1;

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

    /* Receive and play samples. */
    initOutputStream(receiver, config);

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
