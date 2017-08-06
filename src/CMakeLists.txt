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

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)


find_package(Qt5 COMPONENTS Qml Gui)

include("${CMAKE_SOURCE_DIR}/cmake/FindDeezer.cmake")

set (sources_list
main.cpp

deezer_wrapper/deezer_wrapper.cpp
)

set (headers_list
deezer_wrapper/deezer_wrapper.h
)

include_directories(${DEEZER_SDK_INCLUDE_DIR})
link_directories(${DEEZER_SDK_LIBRARY_DIR})

message(${DEEZER_SDK_LIBRARY_DIR})

qt5_add_resources(DEEZY_RESOURCES deezzy.qrc)

add_executable(deezzy ${sources_list} ${headers_list} ${DEEZY_RESOURCES})

qt5_use_modules(deezzy Qml)

target_link_libraries(deezzy
    deezer
    Qt5::Qml
    Qt5::Gui
)

cotire(deezzy)
