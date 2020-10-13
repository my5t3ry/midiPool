cmake_minimum_required (VERSION 3.13)
project (midiPool)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_COMPILER "clang++")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -static -fcoroutines-ts --stdlib=libc++ -Xclang -fconcepts-ts")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")

include_directories (/home/my5t3ry/codeBase/c/boost/boost_1_74_0)
link_directories (/home/my5t3ry/codeBase/c/boost/boost_1_74_0/stage)

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
find_package (Boost 1.74.0 COMPONENTS date_time filesystem system thread)
message (WARNING "libasound found: ${ALSA_FOUND}")
include_directories (${ALSA_INCLUDE_DIRS})
target_link_libraries (client ${ALSA_LIBRARIES} ${Boost_LIBRARIES} pthread)
target_link_libraries (server ${ALSA_LIBRARIES} ${Boost_LIBRARIES} pthread)

#
#TARGET_LINK_LIBRARIES(server
#        pthread
#        )
