#pragma once
#include "build_info.hpp"
#include <cstdlib>
#include <filesystem>
#include <fmt/core.h>
#include <stdexcept>
#include <string>

struct ClientConfig {
    int         port      = 7777;
    std::string server_ip = "127.0.0.1";

    static ClientConfig create_cfg(int argc, const char **argv) {
        ClientConfig cfg;
        cfg.parse_args(argc, argv);
        return cfg;
    };

  private:
    void parse_args(int argc, const char **argv) {
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "-h" || arg == "--help") {
                print_help_and_exit(argv[0]);
            } else if (arg == "-v" || arg == "--version") {
                print_version_and_exit();
            } else if (arg == "-p" || arg == "--port") {
                port = get_int_arg(argv, i, argc);
            } else if (arg == "-i" || arg == "--ip") {
                server_ip = get_string_arg(argv, i, argc);
            } else {
                throw std::runtime_error(fmt::format("Unknown argument: {}", arg));
            }
        }
    };

    std::string get_string_arg(const char **argv, int &i, int argc) {
        if (i + 1 < argc) {
            return argv[++i];
        } else {
            throw std::runtime_error(fmt::format("Parameter {} not found", argv[i]));
        }
    };
    int get_int_arg(const char **argv, int &i, int argc) {
        if (i + 1 >= argc)
            throw std::runtime_error(fmt::format("Parameter {} not found", argv[i]));

        try {
            return std::stoi(argv[++i]);
        } catch (...) {
            throw std::runtime_error(
                fmt::format("Invalid integer for {}: {}", argv[i - 1], argv[i]));
        }
    };

    [[noreturn]] void print_version_and_exit() const {
        fmt::print("{} version {}\n", CLIENT_NAME, CLIENT_VERSION);
        std::exit(0);
    }

    [[noreturn]] void print_help_and_exit(const char *program_name) const {
        fmt::print(
            R"(Usage: {} [options]

Options:
  -h, --help            Show this help and exit
  -p, --port <port>     Server port (default: {})
  -i, --ip   <address>  Server ip address (default: {})
)",
            program_name, port, server_ip);

        std::exit(0);
    };
};
