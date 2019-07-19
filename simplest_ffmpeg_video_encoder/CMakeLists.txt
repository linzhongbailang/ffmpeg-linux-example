cmake_minimum_required(VERSION 2.8)


SET(TARGET_OS linux_x86_64)








message(STATUS "TARGET_OS = ${TARGET_OS}")



project(simplest_ffmpeg_video_test)

include(FindPkgConfig)
pkg_check_modules(PKGS  libjpeg_9c opencv3.4.0-rc libavcodec libavutil libavformat libswscale x264)
include_directories(${PKGS_INCLUDE_DIRS})
link_directories(${PKGS_LIBRARY_DIRS})

#set(CMAKE_C_FLAGS "-pie -fPIE -fpermissive-o")
set(CMAKE_CXX_FLAGS " -std=c++11")




message(STATUS "CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")

message(STATUS "CMAKE_C_FLAGS = " ${CMAKE_C_FLAGS})
message(STATUS "CMAKE_CXX_FLAGS = " ${CMAKE_CXX_FLAGS})
message(STATUS "CMAKE_LD_FLAGS = " ${CMAKE_LD_FLAGS})

add_executable(simplest_ffmpeg_video_test 			simplest_ffmpeg_video_main.c
													simplest_ffmpeg_video_encoder_pure.cpp
													simplest_ffmpeg_h264_decoder_tao.cpp
													simplest_ffmpeg_rtsp_test.cpp
													rtsp_test/ringfifo.c
													rtsp_test/rtputils.c
													rtsp_test/rtspservice.c
													rtsp_test/rtsputils.c
													rtsp_test/RT_Gree_rtsp.c
													)



target_link_libraries(simplest_ffmpeg_video_test ${PKGS_LDFLAGS} -lpthread)





