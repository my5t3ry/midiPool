cmake_minimum_required (VERSION 3.12)
project (midiPool)

unset (Boost_INCLUDE_DIR CACHE)
unset (Boost_LIBRARY_DIRS CACHE)
SET (CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_COMPILER "/usr/bin/clang++")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -D__LINUX_ALSA__ -std=c++20 -fcoroutines-ts -stdlib=libc++")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lstdc++ -lc++ -lc++abi -lasound  -Wl,--no-as-needed -ldl  -lm -lpthread ")
set (BOOST_LIBRARYDIR /home/my5t3ry/codeBase/c/boost/boost_1_74_0/stage/)
set (BOOST_INCLUDEDIR /home/my5t3ry/codeBase/c/boost/boost_1_74_0/)
set (Boost_USE_STATIC_LIBS ON)
set (Boost_USE_MULTITHREADED ON)
set (Boost_USE_STATIC_RUNTIME ON)
find_package (ALSA REQUIRED)
find_package (Boost 1.74.0 COMPONENTS date_time system coroutine)

include_directories (
    .
    /home/my5t3ry/codeBase/c/nuklear-cpp/include
    ${Boost_INCLUDE_DIRS}
    ${ALSA_INCLUDE_DIRS}
    ./audio/signal-estimator/src
    home/my5t3ry/codeBase/c/boost/boost_1_74_0
    /usr/x86_64-linux-gnu/include/c++/8/ext/pb_ds/
)

link_directories (
    ${Boost_LIBRARY_DIRS}
    /home/my5t3ry/codeBase/c/nuklear-cpp/src
    /usr/lib/
    /home/my5t3ry/codeBase/c/boost/boost_1_74_0/stage
    /usr/lib/c++
    /home/my5t3ry/packages/libc++/src/build/lib/
    /home/my5t3ry/packages/libc++/pkg/libc++abi/usr/lib/)

add_executable (server
    midi/RtMidi.cpp
    server/server.cpp)

add_executable (client
    client/client.cpp
    audio/signal-estimator/src/AlsaReader.cpp
    audio/signal-estimator/src/AlsaWriter.cpp
    audio/signal-estimator/src/Frame.cpp
    audio/signal-estimator/src/Config.hpp
    audio/signal-estimator/src/Time.cpp
    audio/signal-estimator/src/AlsaUtils.cpp
    midi/RtMidi.cpp utils/json.hpp)

add_executable (client-ui
    client/client_ui.cpp
    /home/my5t3ry/codeBase/c/nuklear-cpp/include/nuklear_lib.h
    /home/my5t3ry/codeBase/c/nuklear-cpp/include/nuklear.h
    /home/my5t3ry/codeBase/c/nuklear-cpp/src/nuklear_lib.cpp
    )

target_link_libraries (
    client-ui
    SDL2
    GLESv2
    GLdispatch
    nuklear_gles2
)

target_link_libraries (
    client
    ${ALSA_LIBRARIES}
    roc
    uv
    unwind
    sox
    sndio
    asound
    soundio
    dl
    c++abi
    c++
    pthread
    ${Boost_LIBRARIES}
)

target_link_libraries (
    server
    ${ALSA_LIBRARIES}
    roc
    uv
    unwind
    sox
    sndio
    soundio
    c++abi
    dl
    c++
    pthread
    ${Boost_LIBRARIES}
    asound
)

#

