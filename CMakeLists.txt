cmake_minimum_required (VERSION 3.13)
project (midiPool)

SET (CMAKE_FIND_LIBRARY_SUFFIXES ".a")
SET (BUILD_SHARED_LIBRARIES OFF)
set (CMAKE_CXX_COMPILER "clang++")
set (CMAKE_C_COMPILER "clang")
link_directories(/usr/local/lib /usr/lib/)

set (CMAKE_CXX_STANDARD 17)
set (LIBCXX_ENABLE_STATIC ON)
SET (Boost_USE_STATIC_LIBS ON)
SET (Boost_USE_MULTITHREADED ON)
SET (Boost_USE_STATIC_RUNTIME ON)
find_package (Boost 1.74.0 COMPONENTS program_options thread REQUIRED)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}   -DLIBCXX_ENABLE_SHARED=OFF -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON -D_GLIBCXX_USE_DEPRECATED=0  -D_DEBUG -L/usr/lib/ -Wall -lc++abi -lboost_system -std=c++11 -static-libgcc -static-libstdc++  -nodefaultlibs  -lc++ -D__LINUX_ALSA__ -I/usr/include/c++/v1/  -lgcc_eh  -lc++ -lc++abi -lm -lc  -stdlib=libc++ -Wl,--whole-archive -pthread -Wl,--no-whole-archive  -fcoroutines-ts -ldl    -static   -D_GLIBCXX_USE_CXX11_ABI=0 ")
set (CMAKE_CXX_COMPILER "clang++")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -DLIBCXX_ENABLE_SHARED=OFF -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON -v  -D_GLIBCXX_USE_DEPRECATED=0  -lboost_system -lc++abi -L/usr/lib/ -lgcc_eh -static-libgcc -static-libstdc++  -nodefaultlibs  -std=c++11 -lc++ -stdlib=libc++ -Wl,--whole-archive -pthread -Wl,--no-whole-archive -static  -D_GLIBCXX_USE_CXX11_ABI=0")

include_directories (.)

add_executable (server
    RtMidi.cpp
    server.cpp)

add_executable (client
    client.cpp
    RtMidi.cpp json.hpp)

find_package (ALSA REQUIRED)
find_package(Threads REQUIRED)

find_library (my_pthread_static_lib libpthread.a)

message (WARNING "libasound found: ${ALSA_FOUND}")
include_directories (${ALSA_INCLUDE_DIRS})
target_link_libraries (client ${ALSA_LIBRARIES} /usr/lib/libstdc++fs.a /usr/lib/libstdc++.a Threads::Threads stdc++ ${my_pthread_static_lib} Boost::program_options boost_system boost_thread pthread -static-libgcc -static-libstdc++ c++ gcc_eh m dl c c++abi c++experimental -static)
target_link_libraries (server ${ALSA_LIBRARIES} /usr/lib/libstdc++fs.a /usr/lib/libstdc++.a Threads::Threads stdc++ ${my_pthread_static_lib} Boost::program_options  boost_thread  boost_system pthread -static-libgcc -static-libstdc++ c++ gcc_eh m c dl c++abi c++experimental -static)

#
#TARGET_LINK_LIBRARIES(server
#        pthread
#        )
