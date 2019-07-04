cmake_minimum_required(VERSION 2.8)










message(STATUS "TARGET_OS = ${TARGET_OS}")



project(ffmpeg-linux-example)


add_subdirectory(live555_test)

add_subdirectory(simplest_ffmpeg_video_encoder)
add_subdirectory(simplest_mediadata_test)
add_subdirectory(socket_test)
add_subdirectory(udp_test)

