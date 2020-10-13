cmake_minimum_required (VERSION 3.13)
project (midiPool)

SET (CMAKE_FIND_LIBRARY_SUFFIXES ".a")
SET (BUILD_SHARED_LIBRARIES OFF)

set (CMAKE_CXX_STANDARD 17)
set (LIBCXX_ENABLE_STATIC ON)
set (LIBCXXABI_INSTALL_STATIC_LIBRARY ON)
SET (Boost_USE_STATIC_LIBS ON)
SET (Boost_USE_MULTITHREADED ON)
SET (Boost_USE_STATIC_RUNTIME ON)
find_package (Boost 1.74.0 COMPONENTS program_options thread REQUIRED)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -std=c++11 -D_DEBUG -Wall -D__LINUX_ALSA__ -I/usr/include/c++/v1/ -nodefaultlibs   -lc++ -lc++abi -lm -lc  -stdlib=libc++  -fcoroutines-ts -ldl   -static -static-libgcc ")
set (CMAKE_CXX_COMPILER "clang++")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -std=c++0x  -std=c++11-static -stdlib=libc++  ")

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
target_link_libraries (client ${ALSA_LIBRARIES} Boost::program_options pthread c++ gcc_eh m dl c c++abi c++experimental)
target_link_libraries (server ${ALSA_LIBRARIES} Boost::program_options pthread c++ gcc_eh m c dl c++abi c++experimental)

#
#TARGET_LINK_LIBRARIES(server
#        pthread
#        )
