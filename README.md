# midiPool 

### another attempt to build a midi/audio syncing thing with computers

##### long term/no term goals 

- [ ] stable enough that it's usable for online jam sessions
- [X] server/client architecture
- [X] client spawns two virtual midi devices (send/recv.) with rtmidi.
- [X] json msg bus
- [X] users can join broadcast rooms
- [ ] server sends 'bar_sync_msg' to all clients, the msg contains {target_send_timestamp,bpm,position}. client generates a stack of 0xFB and 0xF8 midi messages. this messages will kept in buffer. every message contains a target_send_timestamp. a dedicated clock message spooler pipes the messages into the virtual midi input device. every client receives tick/continue messages simultaneously. clock slave sequencers should be bpm/tick/position synced
- [ ] clients can send midi messages to the virtual midi output device. this messages will be filtered by type (bpm change/pos. change/start/stop/loop area change). this messages will be send to the server which broadcasts it ot all clients. this messages will be send directly to the clients virtual input device. if so that changes other then tempo and sync are received immediately by the clients sequencer     
- [X] 50% done. client spawns virtual audio devices (send/recv.). audio output of all users is send too the server, and returned to the audio input device of all clients
- [ ] audi mixer interface for clients 

##### technologies

- https://www.boost.org. c++, crosscompiling, lightweight, async tcp sockets, latest threading/async/coroutine clang features, static linked binaries probably possible
- https://github.com/thestk/rtmidi /rtaudio crossplatform layer. portaudio/asio/alsa/core audio support
- json/?protobuf. simple msg datastructures. midi messages generated on client side
- https://roc-streaming.org FECFRAME/RTP network audio streaming. c library, lightweight, crosscompiling
- https://github.com/thestk/rtmidi /nanovg. vst client intergration
- udp hole punching. configuration free accessibility for noobs
