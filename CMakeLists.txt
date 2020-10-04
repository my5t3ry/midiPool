cmake_minimum_required (VERSION 3.13)
project (midiPool)

SET (CMAKE_FIND_LIBRARY_SUFFIXES ".a")
SET (BUILD_SHARED_LIBS OFF)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Bstatic -D_DEBUG -Wall -D__LINUX_ALSA__ -I/usr/include/c++/v1/ -nodefaultlibs  -stdlib=libc++ -lc++abi  -lc++experimental  -static-libgcc -static-libstdc++ -fuse-ld=lld -l:libc++experimental.a  -l:libc++.a  -l:libboost_coroutine.a -l:libc++abi.a -l:libunwind.a -lpthread   -l:libfuzzy.a ")
set (CMAKE_CXX_COMPILER "clang++")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -nodefaultlibs  -stdlib=libc++ -lc++abi  -lc++experimental  -static-libgcc -static-libstdc++ -fuse-ld=lld -l:libc++.a -l:libc++abi.a -l:libunwind.a -lpthread  -l:libc++experimental.a   -l:libboost_coroutine.a   -l:libfuzzy.a ")

include_directories (.)

add_executable (server
    RtMidi.cpp
    server.cpp)

add_executable (client
    client.cpp
    RtMidi.cpp signal_handler.hpp json.hpp)

find_package (ALSA REQUIRED)

message (WARNING "libasound found: ${ALSA_FOUND}")
include_directories (${ALSA_INCLUDE_DIRS})
target_link_libraries (client ${ALSA_LIBRARIES} c++ c++experimental pthread boost_thread boost_asio)
target_link_libraries (server ${ALSA_LIBRARIES} c++ c++experimental pthread boost_thread boost_asio)

#
#TARGET_LINK_LIBRARIES(server
#        pthread
#        )
