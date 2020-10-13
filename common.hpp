//
// Created by my5t3ry on 10/4/20.
//

#ifndef MIDIPOOL__COMMON_HPP_
#define MIDIPOOL__COMMON_HPP_
#include "log.hpp"

#include <utility>
#include "RtMidi.h"
#include <boost/asio.hpp>
#include "json.hpp"
#include <deque>
#include <iostream>
#include <memory>
#include <deque>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/thread.hpp>
#include <iostream>
#include <alsa/asoundlib.h>

namespace pt = boost::posix_time;

using boost::thread;
using boost::mutex;
using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::redirect_error;
using boost::asio::use_awaitable;
using boost::asio::ip::tcp;

#if defined(WIN32)
#include <windows.h>
#define SLEEP( milliseconds ) Sleep( (DWORD) milliseconds )
#else // Unix variants
#include <unistd.h>
#define SLEEP(milliseconds) usleep( (unsigned long) (milliseconds * 1000.0) )
#endif
struct midi_message {
  std::vector<unsigned char> *message_bytes;
  long int timestamp;
  int clock_rate;
};

long int static get_posix_timestamp(int offset = 0) {
  return std::time(0);
}
typedef std::deque<nlohmann::json> midi_message_queue;

#include "chat_client.hpp"

#endif //MIDIPOOL__COMMON_HPP_
