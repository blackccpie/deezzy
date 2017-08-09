#!/bin/sh

mkdir -p build_gcc

cd build_gcc
export CC=/usr/bin/gcc-6
export CXX=/usr/bin/g++-6
cmake -DCMAKE_CXX_FLAGS="-march=armv7-a -mfloat-abi=hard -mfpu=neon-vfpv4" -DCMAKE_BUILD_TYPE=Release ..
make
