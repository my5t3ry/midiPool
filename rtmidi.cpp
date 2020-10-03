#include <iostream>
#include <cstdlib>
#include "RtMidi.h"
#include <string>
#include <string>

#include <unistd.h>
#include "libsocket/exception.hpp"
#include "libsocket/inetclientstream.hpp"

/*
 * Sends and receives messages via connected UDP sockets
 */

void mycallback(double deltatime, std::vector<unsigned char> *message, void *userData) {
  unsigned int nBytes = message->size();
  for (unsigned int i = 0; i < nBytes; i++)
    std::cout << "Byte " << i << " = " << (int) message->at(i) << ", ";
  if (nBytes > 0)
    std::cout << "stamp = " << deltatime << std::endl;
}
int main() {
  RtMidiIn *midiin = new RtMidiIn();
  // Check available ports.
  unsigned int nPorts = midiin->getPortCount();
  if (nPorts == 0) {
    std::cout << "No ports available!\n";
  }
  midiin->openVirtualPort("test");
  // Set our callback function.  This should be done immediately after
  // opening the port to avoid having incoming messages written to the
  // queue.
  midiin->setCallback(&mycallback);
  // Don't ignore sysex, timing, or active sensing messages.
  midiin->ignoreTypes(false, false, false);
  using std::string;

  using libsocket::inet_stream;

  string host = "127.0.0.1";
  string port = "1234";
  string answer;

  answer.resize(32);

  try {
    libsocket::inet_stream sock(host, port, LIBSOCKET_IPv4);

    sock >> answer;

    std::cout << answer;

    sock << "Hello back!\n";

    // sock is closed here automatically!
  }
  catch (const libsocket::socket_exception &exc) {
    std::cerr << exc.mesg;
  }

}
