cmake_minimum_required (VERSION 3.13)
project (midiPool)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_DEBUG -Wall -D__LINUX_ALSA__ -I/usr/include/c++/v1/ -nodefaultlibs -stdlib=libc++ -lc++ -lc++abi -lm -lc -lgcc_s -lgcc -lc++experimental -fcoroutines-ts  ")
set (CMAKE_CXX_COMPILER "clang++")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -nodefaultlibs -lc++ -lc++abi -lm -lc -lgcc_s -lgcc -stdlib=libc++ -lc++abi  -lc++experimental")

include_directories (.)

add_executable (server
    RtMidi.cpp
    server.cpp)

add_executable (client
    client.cpp
    RtMidi.cpp json.hpp)

find_package (ALSA REQUIRED)

message (WARNING "libasound found: ${ALSA_FOUND}")
include_directories (${ALSA_INCLUDE_DIRS})
target_link_libraries (client ${ALSA_LIBRARIES} pthread c++ c++abi c++experimental boost_thread)
target_link_libraries (server ${ALSA_LIBRARIES} pthread c++ c++abi c++experimental boost_thread)

#
#TARGET_LINK_LIBRARIES(server
#        pthread
#        )
