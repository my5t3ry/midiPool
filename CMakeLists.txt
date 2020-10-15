cmake_minimum_required (VERSION 3.12)
project (midiPool)

SET (CMAKE_FIND_LIBRARY_SUFFIXES ".a")
#SET (BUILD_SHARED_LIBRARIES OFF)K
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_COMPILER "clang++")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang  -D__LINUX_ALSA__ -std=c++20 -fcoroutines-ts -stdlib=libc++")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lstdc++ -lc++ -lc++abi -lasound -lroc -Wl,--no-as-needed -ldl  -lm -lpthread -static")

include_directories (.)

add_executable (server
    audio/audio_server.hpp
    RtMidi.cpp
    server.cpp)

add_executable (client
    client.cpp
    RtMidi.cpp json.hpp)

#add_executable (audio_server
#    audio/audio_server.c
#    )
find_package (ALSA REQUIRED)

unset (Boost_INCLUDE_DIR CACHE)
unset (Boost_LIBRARY_DIRS CACHE)
set (BOOST_LIBRARYDIR /home/my5t3ry/codeBase/c/boost/boost_1_74_0/stage/)
set (BOOST_INCLUDEDIR /home/my5t3ry/codeBase/c/boost/boost_1_74_0/)
set (Boost_USE_STATIC_LIBS ON) # only find static libs
set (Boost_USE_MULTITHREADED ON)
set (Boost_USE_STATIC_RUNTIME ON)

find_package (Boost 1.74.0 COMPONENTS date_time system coroutine)

link_directories (${Boost_LIBRARY_DIRS} audio/roc-toolkit/src/lib/include /usr/lib64  /home/my5t3ry/codeBase/c/boost/boost_1_74_0/stage /usr/lib/c++ /home/my5t3ry/packages/libc++/src/build/lib/ /home/my5t3ry/packages/libc++/pkg/libc++abi/usr/lib/)

message (WARNING "libasound found: ${ALSA_FOUND}  ${ALSA_LIBRARIES}")
include_directories (${Boost_INCLUDE_DIRS} ${ALSA_INCLUDE_DIRS} /home/my5t3ry/codeBase/c/boost/boost_1_74_0 /usr/x86_64-linux-gnu/include/c++/8/ext/pb_ds/ .)

target_link_libraries (client ${ALSA_LIBRARIES} asound dl c++abi c++ pthread ${Boost_LIBRARIES} -static)
target_link_libraries (server ${ALSA_LIBRARIES} roc
    c++abi dl c++ pthread ${Boost_LIBRARIES} asound -static)

#
#TARGET_LINK_LIBRARIES(server
#        pthread
#        )
