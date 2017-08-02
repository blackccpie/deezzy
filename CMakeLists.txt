#The MIT License
#
#Copyright (c) 2017-2017 Albert Murienne
#
#Permission is hereby granted, free of charge, to any person obtaining a copy
#of this software and associated documentation files (the "Software"), to deal
#in the Software without restriction, including without limitation the rights
#to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#copies of the Software, and to permit persons to whom the Software is
#furnished to do so, subject to the following conditions:
#
#The above copyright notice and this permission notice shall be included in
#all copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
#AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#THE SOFTWARE.

cmake_minimum_required (VERSION 3.2)
project (deezzy)

include("${CMAKE_SOURCE_DIR}/cmake/FindDeezer.cmake")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXTRA_C_FLAGS "${EXTRA_C_FLAGS} -Wall -Wpedantic -Wno-narrowing")

# Set compiler options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_C_FLAGS}")

set (sources_list
src/main.cpp
)

include_directories("${CMAKE_SOURCE_DIR}/src" ${DEEZER_SDK_INCLUDE_DIR})
link_directories(${DEEZER_SDK_LIBRARY_DIR})

message(${DEEZER_SDK_LIBRARY_DIR})

add_executable(deezzy ${sources_list} ${headers_list})

target_link_libraries(deezzy
    deezer
)
