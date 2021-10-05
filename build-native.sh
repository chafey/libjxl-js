#!/bin/sh
mkdir -p build-native
(cd build-native && cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF ..)
#(cd build && cmake ..)
(cd build-native && make VERBOSE=1 -j$(nproc))
(build-native/test/cpp/cpptest)