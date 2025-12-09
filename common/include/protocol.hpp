#pragma once
#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace Proto {

enum ID : uint8_t {
    LOGIN,
    MESSAGE,
};

struct PacketHeader {
    Proto::ID id;
    uint16_t  length;
    PacketHeader(Proto::ID p_id, uint16_t p_length) {
        id = p_id;
        // host to network byte ordering (16bit)
        length = htons(p_length);
    };
    PacketHeader() : id(), length(){};
};

struct LoginRequest {
    std::string username;

    std::vector<uint8_t> serialize() const {
        return std::vector<uint8_t>(username.begin(), username.end());
    }

    static std::vector<uint8_t> serialize(const std::string &username) {
        return std::vector<uint8_t>(username.begin(), username.end());
    }

    static LoginRequest deserialize(const std::vector<uint8_t> &data) {
        return LoginRequest{std::string(data.begin(), data.end())};
    }
};

struct Message {
    std::string message;

    std::vector<uint8_t> serialize() const {
        return std::vector<uint8_t>(message.begin(), message.end());
    }

    static std::vector<uint8_t> serialize(const std::string &message) {
        return std::vector<uint8_t>(message.begin(), message.end());
    }

    static Message deserialize(const std::vector<uint8_t> &data) {
        return Message{std::string(data.begin(), data.end())};
    }
};

} // namespace Proto