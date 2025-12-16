#pragma once
#include "build_info.hpp"
#include "s_logger.hpp"
#include <cstdlib>
#include <filesystem>
#include <fmt/core.h>
#include <stdexcept>
#include <string>

struct ServerConfig {
    int         port          = 7777;
    std::string db_address    = "tcp://127.0.0.1";
    std::string db_user       = "convo";
    std::string db_password   = "Qweasdzxc1!";
    std::string db_schema     = "convo";
    std::string cert_pem_path = "/home/piotr/Projects/ChatTerminal/cert/server.pem";
    std::string cert_key_path = "/home/piotr/Projects/ChatTerminal/cert/server.key";

    static ServerConfig create_cfg(int argc, const char **argv) {
        ServerConfig cfg;
        cfg.load_from_env();
        cfg.parse_args(argc, argv);
        return cfg;
    };

  private:
    void load_from_env() {
        if (const char *val = std::getenv("SERVER_PORT"))
            port = std::stoi(val);
        if (const char *val = std::getenv("DB_ADDRESS"))
            db_address = val;
        if (const char *val = std::getenv("DB_USER"))
            db_user = val;
        if (const char *val = std::getenv("DB_PASSWORD"))
            db_password = val;
        if (const char *val = std::getenv("DB_SCHEMA"))
            db_schema = val;
        if (const char *val = std::getenv("CERT_PEM_PATH"))
            cert_pem_path = val;
        if (const char *val = std::getenv("CERT_KEY_PATH"))
            cert_key_path = val;
    };
    void parse_args(int argc, const char **argv) {
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "-h" || arg == "--help") {
                print_help_and_exit(argv[0]);
            } else if (arg == "-v" || arg == "--version") {
                print_version_and_exit();
            } else if (arg == "-p" || arg == "--port") {
                port = get_int_arg(argv, i, argc);
            } else if (arg == "-dA") {
                db_address = get_string_arg(argv, i, argc);
            } else if (arg == "-dU") {
                db_user = get_string_arg(argv, i, argc);
            } else if (arg == "-dP") {
                db_password = get_string_arg(argv, i, argc);
            } else if (arg == "-dS") {
                db_schema = get_string_arg(argv, i, argc);
            } else if (arg == "-cP") {
                cert_pem_path = std::filesystem::absolute(get_string_arg(argv, i, argc));
            } else if (arg == "-cK") {
                cert_key_path = std::filesystem::absolute(get_string_arg(argv, i, argc));
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
        fmt::print("{} version {}\n", SERVER_NAME, SERVER_VERSION);
        std::exit(0);
    }

    [[noreturn]] void print_help_and_exit(const char *program_name) const {
        fmt::print(
            R"(Usage: {} [options]

Options:
  -h, --help            Show this help and exit
  -p, --port <port>     Server port (default: {})
  -dA <address>         Database address (default: {})
  -dU <user>            Database user (default: {})
  -dP <password>        Database password
  -dS <schema>          Database schema (default: {})
  -cP <path>            TLS certificate PEM path
  -cK <path>            TLS certificate private key path

Environment variables:
  SERVER_PORT
  DB_ADDRESS
  DB_USER
  DB_PASSWORD
  DB_SCHEMA
  CERT_PEM_PATH
  CERT_KEY_PATH
)",
            program_name, port, db_address, db_user, db_schema);

        std::exit(0);
    };
};
