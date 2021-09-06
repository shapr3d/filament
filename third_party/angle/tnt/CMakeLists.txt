cmake_minimum_required(VERSION 3.19)
project(angle)

set(TARGET angle)

add_library(angle INTERFACE)
target_include_directories(angle INTERFACE ${OPENGL_INCLUDE_DIR})
