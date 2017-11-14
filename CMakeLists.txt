cmake_minimum_required(VERSION 3.6)
project(tiny_timeusage)

set(CMAKE_CXX_STANDARD 11)

#
# Simplicify file name
#
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

#
# Enable the macro
#
add_definitions("-D_BLOCK_TIME_CONSUMING_ENABLE")

set(SOURCE_FILES example.cpp)

add_executable(timeusage ${SOURCE_FILES})