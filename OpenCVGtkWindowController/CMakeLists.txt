cmake_minimum_required(VERSION 3.10)

#
# check for dependencies supplied by parent project
#

if(NOT opencv_POPULATED OR OpenCV_POPULATED)
  message(FATAL_ERROR "${PROJECT_NAME}: missing dependency OpenCV")
endif()

if(NOT GTK3_FOUND)
  message(FATAL_ERROR "${PROJECT_NAME}: missing dependency GTK3")
endif()

#
# config shared library
#

add_library(OpenCVGtkWindowController SHARED
  OpenCVGtkWindowController.cpp
)
# `--std=c++17` (not gnu++17)
set_target_properties(OpenCVGtkWindowController PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)
target_include_directories(OpenCVGtkWindowController PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${OpenCV_INCLUDE_DIRS}
  ${GTK3_INCLUDE_DIRS}
)

target_link_libraries(OpenCVGtkWindowController
  ${OpenCV_LIBRARIES}
  ${GTK3_LINK_LIBRARIES}
)
