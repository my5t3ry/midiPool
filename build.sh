#!/bin/bash
g++ -Wall -D__LINUX_ALSA__ -o client client.cpp RtMidi.cpp -lasound -lpthread
g++ -o server server.cpp -lpthread


