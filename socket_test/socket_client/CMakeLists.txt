cmake_minimum_required(VERSION 2.8)


SET(TARGET_OS linux_x86_64)








message(STATUS "TARGET_OS = ${TARGET_OS}")



project(socket_client)

include(FindPkgConfig)
pkg_check_modules(PKGS  libjpeg_9c opencv3.4.0-rc)
include_directories(${PKGS_INCLUDE_DIRS})
link_directories(${PKGS_LIBRARY_DIRS})






message(STATUS "CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")

message(STATUS "CMAKE_C_FLAGS = " ${CMAKE_C_FLAGS})
message(STATUS "CMAKE_CXX_FLAGS = " ${CMAKE_CXX_FLAGS})
message(STATUS "CMAKE_LD_FLAGS = " ${CMAKE_LD_FLAGS})

add_executable(socket_client 	socket_client.c
										)
target_link_libraries(socket_client ${PKGS_LDFLAGS})





