cmake_minimum_required (VERSION 3.17)
project (midiPool)

SET (CMAKE_FIND_LIBRARY_SUFFIXES ".a")
#SET (BUILD_SHARED_LIBRARIES OFF)
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_COMPILER "clang++")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -D__LINUX_ALSA__ -std=c++20 -fcoroutines-ts -stdlib=libc++")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lstdc++ -lc++ -lc++abi -lasound  -Wl,--no-as-needed -ldl  -lm -pthread -static")

include_directories (.)

add_executable (server
    RtMidi.cpp
    server.cpp)

add_executable (client
    client.cpp
    RtMidi.cpp json.hpp)

find_package (ALSA REQUIRED)

set (Boost_USE_STATIC_LIBS ON) # only find static libs
set (Boost_USE_MULTITHREADED ON)
set (Boost_USE_STATIC_RUNTIME ON)
unset (Boost_INCLUDE_DIR CACHE)
unset (Boost_LIBRARY_DIRS CACHE)
find_package (Boost 1.74.0 COMPONENTS date_time system coroutine)

include_directories (${Boost_INCLUDE_DIRS} /home/my5t3ry/codeBase/c/boost/boost_1_74_0 .)
link_directories (${Boost_LIBRARY_DIRS} /home/my5t3ry/codeBase/c/boost/boost_1_74_0/stage /usr/lib/c++ /home/my5t3ry/packages/libc++/src/build/lib/ /home/my5t3ry/packages/libc++/pkg/libc++abi/usr/lib/)

message (WARNING "libasound found: ${ALSA_FOUND}  ${ALSA_LIBRARIES}")
include_directories (${ALSA_INCLUDE_DIRS})
target_link_libraries (client ${ALSA_LIBRARIES} asound dl c++abi c++ pthread ${Boost_LIBRARIES} -static)
target_link_libraries (server ${ALSA_LIBRARIES}
    c++abi dl c++ pthread ${Boost_LIBRARIES} asound -static)

#
#TARGET_LINK_LIBRARIES(server
#        pthread
#        )
