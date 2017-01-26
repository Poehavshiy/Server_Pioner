cmake_minimum_required(VERSION 3.6)
project(Server_Pioner)

set(CMAKE_CXX_STANDARD 11)
find_package (Threads)
include_directories("/home/nikita/networking/libevent-2.0.22-stable/include")



set(SOURCE_FILES main.cpp Thread_Poll.cpp Thread_Poll.h)
add_executable(Server_Pioner ${SOURCE_FILES})
target_link_libraries(Server_Pioner pthread)
target_link_libraries(Server_Pioner event_core)
