cmake_minimum_required(VERSION 3.10)

project(Two_Videos)


find_package(PkgConfig REQUIRED)
pkg_check_modules(AVFORMAT libavformat REQUIRED)
pkg_check_modules(AVCODEC libavcodec REQUIRED)
pkg_check_modules(AVUTIL libavutil REQUIRED)
pkg_check_modules(SWSCALE libswscale REQUIRED)
find_package(OpenCV REQUIRED)

add_subdirectory(OpenCVGtkWindowController)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(../)

include_directories(${AVFORMAT_INCLUDE_DIRS}
                    ${AVCODEC_INCLUDE_DIRS}
                    ${AVUTIL_INCLUDE_DIRS}
                    ${SWSCALE_INCLUDE_DIRS}
                    ${OpenCV_INCLUDE_DIRS})


add_executable(${PROJECT_NAME} Two_Videos_Noise.cpp mixer_processor.cpp )

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries(${PROJECT_NAME} ${AVFORMAT_LIBRARIES}
                      ${AVCODEC_LIBRARIES}
                      ${AVUTIL_LIBRARIES}
                      ${SWSCALE_LIBRARIES}
                      ${OpenCV_LIBS})

target_compile_definitions(${PROJECT_NAME} PUBLIC
  # quotes passed into macros (source expects string literal)
  TIF_004_PATH_SL="${PROJECT_SOURCE_DIR}/004.tif"
  TIF_043_PATH_SL="${PROJECT_SOURCE_DIR}/043.tif"
  TIF_169_PATH_SL="${PROJECT_SOURCE_DIR}/169.tif"
)






# to build xcode project
#   cd xbuild
#   cmake .. -GXcode



