cmake_minimum_required(VERSION 2.8)


#SET(TARGET_OS linux_x86_64)








message(STATUS "TARGET_OS = ${TARGET_OS}")



project(socket_test)



add_subdirectory(socket_client)
add_subdirectory(socket_server)

