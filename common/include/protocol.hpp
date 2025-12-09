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
    std::string username;
    std::string message;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> payload;

        uint8_t unameLength = static_cast<uint8_t>(std::min<size_t>(username.size(), 255));
        payload.push_back(unameLength);
        payload.insert(payload.end(), username.begin(), username.begin() + unameLength);
        payload.insert(payload.end(), message.begin(), message.end());

        return payload;
    }

    static std::vector<uint8_t> serialize(const std::string &username, const std::string &message) {
        std::vector<uint8_t> payload;

        uint8_t unameLength = static_cast<uint8_t>(std::min<size_t>(username.size(), 255));
        payload.push_back(unameLength);
        payload.insert(payload.end(), username.begin(), username.begin() + unameLength);
        payload.insert(payload.end(), message.begin(), message.end());

        return payload;
    }

    static Message deserialize(const std::vector<uint8_t> &data) {
        Message msg;

        if (data.empty())
            return msg;

        uint8_t unameLength = data[0];
        if (data.size() < 1 + unameLength)
            return msg;
        // ! TODO: need to add sanity check data.size() >= 1 + unameLength
        msg.username = std::string(data.begin() + 1, data.begin() + 1 + unameLength);
        msg.message  = std::string(data.begin() + 1 + unameLength, data.end());

        return msg;
    }
};

} // namespace Proto