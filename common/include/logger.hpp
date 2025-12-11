#pragma once
#include <cstdint>
#include <format>
#include <iostream>
#include <mutex>

class Logger {
  public:
    static inline void no_prefix(const std::string &msg) {
        std::lock_guard<std::mutex> lock(get_mutex());
        std::cout << msg << std::endl;
    };

    static inline void server_message(const std::string &msg) {
        std::lock_guard<std::mutex> lock(get_mutex());
        std::cout << "\033[97m[Server] " << msg << "\033[0m" << std::endl;
    };

    static inline void client_message(const std::string &client, const std::string &msg) {
        std::lock_guard<std::mutex> lock(get_mutex());
        std::cout << std::format("\033[90m[ MESS ] {} | {}\033[0m", client, msg) << std::endl;
    };

    static inline void info(const std::string &msg) {
        std::lock_guard<std::mutex> lock(get_mutex());
        std::cout << "\033[36m[ INFO ] " << msg << "\033[0m" << std::endl;
    };

    static inline void warn(const std::string &msg) {
        std::lock_guard<std::mutex> lock(get_mutex());
        std::cout << "\033[33m[ WARN ] " << msg << "\033[0m" << std::endl;
    };

    static inline void error(const std::string &msg) {
        std::lock_guard<std::mutex> lock(get_mutex());
        std::cout << "\033[31m[ ERRO ] " << msg << "\033[0m" << std::endl;
    };

  private:
    static std::mutex &get_mutex() {
        static std::mutex mtx;
        return mtx;
    }
};