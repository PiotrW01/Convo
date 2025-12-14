file(GLOB CLIENT_SOURCES "src/*.cpp")

add_executable(client
    ${CLIENT_SOURCES}
)

target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(client PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    common
    ftxui-component
    ftxui-dom
    ftxui-screen
    fmt
)