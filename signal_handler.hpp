//
// Created by my5t3ry on 10/3/20.
//


#include <atomic>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

class signal_handler {
 public:
  signal_handler() : is_signal_received_(false) {
    //signals_::remove();
    std::cout << "constructor" << std::endl;
  }

  void init() {
    std::cout << "Init " << std::endl;
    boost::asio::signal_set signals(signalService_, SIGINT, SIGTERM, SIGQUIT);
    signals.async_wait(&signal_handler::handleStop);
    boost::thread signalThread(boost::bind(&boost::asio::io_service::run, &signalService_));
    std::cout << "Init Completed" << std::endl;
  }

  bool isSignalReceived() const {
    return is_signal_received_;
  }

 private:
  std::atomic<bool> is_signal_received_;
  boost::asio::io_service signalService_;

  static void handleStop(
      const boost::system::error_code &error,
      int signal_number) {
    if (signal_number != 0) {
      std::cout << printf("Executing Safe Shutdown [%d]", signal_number);
      exit(0);
    }
  }
};

