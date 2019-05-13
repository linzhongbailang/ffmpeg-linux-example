cmake_minimum_required(VERSION 2.8)










message(STATUS "TARGET_OS = ${TARGET_OS}")



project(udp_test)


add_subdirectory(udp_client)
add_subdirectory(udp_server)

