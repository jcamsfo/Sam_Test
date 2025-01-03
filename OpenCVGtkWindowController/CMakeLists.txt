cmake_minimum_required(VERSION 3.10)

#
# check for dependencies
#

find_package(OpenCV REQUIRED)

# GTK3 seems to only support cmake via pkg-config, see:
#   https://docs.gtk.org/gtk3/getting_started.html
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

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
  ${OpenCV_LIBS}
  ${GTK3_LINK_LIBRARIES}
)
