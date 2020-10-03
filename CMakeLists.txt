cmake_minimum_required (VERSION 3.13)
project (midiPool)

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -D__LINUX_ALSA__")

include_directories (.)

add_executable (server
    RtMidi.cpp
    server.cpp)

add_executable (client
    client.cpp
    RtMidi.cpp signal_handler.hpp json.hpp )

find_package (ALSA REQUIRED)

message (WARNING "libasound found: ${ALSA_FOUND}")
include_directories (${ALSA_INCLUDE_DIRS})
target_link_libraries (client ${ALSA_LIBRARIES} pthread boost_thread)
target_link_libraries (server ${ALSA_LIBRARIES} pthread boost_thread)

#
#TARGET_LINK_LIBRARIES(server
#        pthread
#        )
