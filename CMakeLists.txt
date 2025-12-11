cmake_minimum_required(VERSION 3.10)
project(ClientServer)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(FTXUI_DIR "${CMAKE_SOURCE_DIR}/external/ftxui")
include_directories(${FTXUI_DIR}/include)
link_directories(${FTXUI_DIR}/lib)

set(FMT_DIR "${CMAKE_SOURCE_DIR}/external/fmt")

add_subdirectory(${FMT_DIR})

# valid build targets: common client server
add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)
