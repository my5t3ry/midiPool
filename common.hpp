//
// Created by my5t3ry on 10/4/20.
//

#ifndef MIDIPOOL__COMMON_HPP_
#define MIDIPOOL__COMMON_HPP_

#include <utility>
#include "RtMidi.h"
#include <boost/asio.hpp>
#include "json.hpp"
#include <deque>
#include <iostream>
#include <memory>
#include <iostream>
#include <deque>
#include <memory>
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
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/thread.hpp>
namespace logging = boost::log;

void initLogger() {
  logging::core::get()->set_filter
      (
          logging::trivial::severity >= logging::trivial::info
      );
}

#endif //MIDIPOOL__COMMON_HPP_
