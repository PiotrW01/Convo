cmake_minimum_required(VERSION 3.10)
project(ClientServer)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)
if(NOT OpenSSL_FOUND)
    message(FATAL_ERROR "Project requires OpenSSL, please install it.")
endif()

set(FTXUI_DIR "${CMAKE_SOURCE_DIR}/external/ftxui")
include_directories(${FTXUI_DIR}/include)
link_directories(${FTXUI_DIR}/lib)

set(FMT_DIR "${CMAKE_SOURCE_DIR}/external/fmt")
add_subdirectory(${FMT_DIR})

include_directories("${CMAKE_SOURCE_DIR}/external/asio-1.36.0/include")

#add_library(bcrypt STATIC external/bcrypt/bcrypt.c)
#target_include_directories(bcrypt PUBLIC external/bcrypt)

# valid build targets: common client server
add_subdirectory(external/libbcrypt)
add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)
