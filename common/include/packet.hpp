#pragma once
#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace Proto {
using Payload     = std::vector<uint8_t>;
using Bytes       = std::vector<uint8_t>;
using PayloadSize = uint16_t;

enum PACKET_ID : uint8_t {
    LOGIN,
    MESSAGE,
    ERROR,
};

enum PACKET_ERROR : uint8_t { ERROR_HEADER_SIZE, ERROR_OTHER = 200 };

enum class Endianness { NETWORK_TO_HOST, HOST_TO_NETWORK };

inline bool is_header_ready(const Bytes &buffer) {
    return (buffer.size() >= sizeof(PacketHeader));
}

inline bool is_packet_ready(const Bytes &buffer, const PayloadSize &payload_size) {
    return buffer.size() >= sizeof(PacketHeader) + payload_size;
}

inline Payload get_payload(const Bytes &buffer, const PayloadSize &payload_size) {
    return Payload(buffer.begin() + sizeof(PacketHeader),
                   buffer.begin() + sizeof(PacketHeader) + payload_size);
}

inline void remove_packet(Bytes &buffer, const PayloadSize payload_size) {
    buffer.erase(buffer.begin(), buffer.begin() + sizeof(PacketHeader) + payload_size);
}

#pragma pack(push, 1)
struct PacketHeader {
    Proto::PACKET_ID id;
    PayloadSize      payload_size;
    PacketHeader() : id(), payload_size(){};
    PacketHeader(PACKET_ID id) : id(id), payload_size(){};
    PacketHeader(const Bytes &buffer, Endianness mode) {
        if (buffer.size() < sizeof(PacketHeader)) {
            id           = PACKET_ID::ERROR;
            payload_size = 0;
            return;
        }

        id = static_cast<PACKET_ID>(buffer[0]);
        if (mode == Endianness::NETWORK_TO_HOST) {
            payload_size =
                ntohs(*reinterpret_cast<const PayloadSize *>(buffer.data() + sizeof(PACKET_ID)));
        } else {
            payload_size =
                htons(*reinterpret_cast<const PayloadSize *>(buffer.data() + sizeof(PACKET_ID)));
        }
    };

    Payload serialize() const {
        Payload buffer(3);
        buffer[0]                        = static_cast<uint8_t>(id);
        PayloadSize network_payload_size = htons(payload_size);
        std::memcpy(buffer.data() + 1, &network_payload_size, sizeof(PayloadSize));
        return buffer;
    }
};
#pragma pack(pop)

struct Packet {
    virtual Bytes serialize()                       = 0;
    virtual void  deserialize(const Bytes &payload) = 0;

  protected:
    PacketHeader hdr{};
};

struct Message : Packet {
    std::string username;
    std::string message;
    Message() { hdr.id = PACKET_ID::MESSAGE; };
    Bytes serialize() override {
        // calculate sizes
        uint8_t     username_length = static_cast<uint8_t>(std::min<size_t>(username.size(), 255));
        PayloadSize payload_size    = 1 + username_length + message.size();

        // construct the packet
        hdr.payload_size = payload_size;
        Bytes packet     = hdr.serialize();

        packet.push_back(username_length);
        packet.insert(packet.end(), username.begin(), username.begin() + username_length);
        packet.insert(packet.end(), message.begin(), message.end());
        return packet;
    };
    void deserialize(const Bytes &payload) {
        uint8_t username_length = payload[0];

        username = std::string(payload.begin() + 1, payload.begin() + 1 + username_length);
        message  = std::string(payload.begin() + 1 + username_length, payload.end());
    };
};

struct LoginRequest : Packet {
    std::string username;
    LoginRequest() { hdr.id = PACKET_ID::LOGIN; };
    Bytes serialize() override {
        uint8_t     username_length = static_cast<uint8_t>(std::min<size_t>(username.size(), 255));
        PayloadSize payload_size    = username_length;

        hdr.payload_size = payload_size;
        Bytes packet     = hdr.serialize();

        packet.insert(packet.end(), username.begin(), username.begin() + username_length);
        return packet;
    };
    void deserialize(const Bytes &payload) {
        username = std::string(payload.begin(), payload.end());
    }
};

struct Error : Packet {
    uint8_t     error_code;
    std::string error_description;
    Error() { hdr.id = PACKET_ID::ERROR; };
    Bytes serialize() override {
        PayloadSize payload_size = sizeof(error_code) + error_description.size();

        hdr.payload_size = payload_size;
        Bytes packet     = hdr.serialize();

        packet.push_back(error_code);
        packet.insert(packet.end(), error_description.begin(), error_description.end());
        return packet;
    };
    void deserialize(const Bytes &payload) {
        error_code        = payload[0];
        error_description = std::string(payload.begin() + 1, payload.end());
    }
};
} // namespace Proto