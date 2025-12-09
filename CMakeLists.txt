cmake_minimum_required(VERSION 3.10)
project(ClientServer)
set(CMAKE_CXX_STANDARD 17)

set(FTXUI_DIR "${CMAKE_SOURCE_DIR}/external/ftxui")
include_directories(${FTXUI_DIR}/include)
link_directories(${FTXUI_DIR}/lib)

# valid build targets: common client server
add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)
