cmake_minimum_required (VERSION 3.12)
project (midiPool)

SET (CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_COMPILER "clang++")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang   -D__LINUX_ALSA__ -std=c++20 -fcoroutines-ts -stdlib=libc++")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lstdc++ -lc++ -lc++abi -lasound  -Wl,--no-as-needed -ldl  -lm -lpthread ")

include_directories (.)

add_executable (server
    midi/RtMidi.cpp
    server/server.cpp)

add_executable (client
    client/client.cpp
    midi/RtMidi.cpp utils/json.hpp)

unset (Boost_INCLUDE_DIR CACHE)
unset (Boost_LIBRARY_DIRS CACHE)
set (BOOST_LIBRARYDIR /home/my5t3ry/codeBase/c/boost/boost_1_74_0/stage/)
set (BOOST_INCLUDEDIR /home/my5t3ry/codeBase/c/boost/boost_1_74_0/)
set (Boost_USE_STATIC_LIBS ON) # only find static libs
set (Boost_USE_MULTITHREADED ON)
set (Boost_USE_STATIC_RUNTIME ON)

find_package (ALSA REQUIRED)
find_package (Boost 1.74.0 COMPONENTS date_time system coroutine)
message (WARNING "libasound found: ${ALSA_FOUND}  ${ALSA_LIBRARIES}")

link_directories (${Boost_LIBRARY_DIRS} /usr/lib/ /home/my5t3ry/codeBase/c/boost/boost_1_74_0/stage /usr/lib/c++ /home/my5t3ry/packages/libc++/src/build/lib/ /home/my5t3ry/packages/libc++/pkg/libc++abi/usr/lib/)
include_directories (${Boost_INCLUDE_DIRS} ${ALSA_INCLUDE_DIRS} home/my5t3ry/codeBase/c/boost/boost_1_74_0 /usr/x86_64-linux-gnu/include/c++/8/ext/pb_ds/)

target_link_libraries (client ${ALSA_LIBRARIES} roc uv unwind asound dl c++abi c++ pthread ${Boost_LIBRARIES})
target_link_libraries (server ${ALSA_LIBRARIES} roc uv unwind
    c++abi dl c++ pthread ${Boost_LIBRARIES} asound)

#

