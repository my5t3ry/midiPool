cmake_minimum_required (VERSION 3.19)
project (midiPool)

SET (CMAKE_FIND_LIBRARY_SUFFIXES ".a")
SET (BUILD_SHARED_LIBRARIES OFF)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_COMPILER "clang++")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fcoroutines-ts -stdlib=libc++  -lc++experimental -Xclang  ")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lstdc++ -lc++abi ")

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
set (Boost_USE_STATIC_RUNTIME OFF)
unset (Boost_INCLUDE_DIR CACHE)
unset (Boost_LIBRARY_DIRS CACHE)
find_package (Boost 1.74.0 COMPONENTS date_time thread coroutine)

include_directories (${Boost_INCLUDE_DIRS} /home/my5t3ry/codeBase/c/boost/boost_1_74_0 .)
link_directories (${Boost_LIBRARY_DIRS} /home/my5t3ry/codeBase/c/boost/boost_1_74_0/stage /usr/lib/c++ /home/my5t3ry/packages/libc++/src/build/lib/ /home/my5t3ry/packages/libc++/pkg/libc++abi/usr/lib/)

message (WARNING "libasound found: ${ALSA_FOUND}")
include_directories (${ALSA_INCLUDE_DIRS})
target_link_libraries (client ${ALSA_LIBRARIES} stdc++ ${Boost_LIBRARIES})
target_link_libraries (server ${ALSA_LIBRARIES} stdc++ ${Boost_LIBRARIES})

#
#TARGET_LINK_LIBRARIES(server
#        pthread
#        )
