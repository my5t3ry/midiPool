//
// Created by my5t3ry on 10/4/20.
//

#ifndef MIDIPOOL__COMMON_HPP_
#define MIDIPOOL__COMMON_HPP_
#define BOOST_ASIO_HAS_CO_AWAIT 1

#include "log.hpp"
#include <alsa/asoundlib.h>

#include <any>
#include <utility>
#include "midi/RtMidi.h"
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
#include <boost/lexical_cast.hpp>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <iostream>

namespace pt = boost::posix_time;

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

long int static get_posix_timestamp(int offset = 0) {
  return std::time(0) + offset;
}
typedef std::deque<nlohmann::json> midi_message_queue;

#include "server_config.hpp"
#include "structs.hpp"

#include "audio/audio_server_socket.hpp"
#include "audio/audio_transmitter.hpp"

#include "client/client_connection.hpp"

#endif //MIDIPOOL__COMMON_HPP_
