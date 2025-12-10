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

enum ID : uint8_t {
    LOGIN,
    MESSAGE,
    ERROR_HEADER_SIZE = 100,
};

enum class Endianness { NETWORK_TO_HOST, HOST_TO_NETWORK };

/**
 * Proto::PacketHandler, lepszą nazwe wymyślić może
 * function callback ktory otrzymuje hdr.id wraz z payload
 * albo func callback dla kazdego rodzaju pakietu ktory otrzymuje
 * gotowy skonstruowany pakiet z payload'u
 *
 * byłoby zamiast switch case od hdr.id w klience i na serverze
 */

#pragma pack(push, 1)
struct PacketHeader {
    Proto::ID   id;
    PayloadSize length;
    PacketHeader() : id(), length(){};
    PacketHeader(Proto::ID p_id, PayloadSize p_length, Endianness mode) {
        id = p_id;
        if (mode == Endianness::NETWORK_TO_HOST) {
            length = ntohs(p_length);
        } else {
            length = htons(p_length);
        }
    };
    PacketHeader(const Bytes &buffer, Endianness mode) {
        if (buffer.size() < sizeof(PacketHeader)) {
            id     = ID::ERROR_HEADER_SIZE;
            length = 0;
            return;
        }

        id = static_cast<ID>(buffer[0]);
        if (mode == Endianness::NETWORK_TO_HOST) {
            length = ntohs(*reinterpret_cast<const PayloadSize *>(buffer.data() + sizeof(ID)));
        } else {
            length = htons(*reinterpret_cast<const PayloadSize *>(buffer.data() + sizeof(ID)));
        }
    };

    Payload serialize() const {
        Payload buffer(3);
        buffer[0]                  = static_cast<uint8_t>(id);
        PayloadSize network_length = htons(length);
        std::memcpy(buffer.data() + 1, &network_length, sizeof(network_length));
        return buffer;
    }
};
#pragma pack(pop)

struct LoginRequest {
    std::string username;

    Proto::Payload serialize() const {
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

    Payload serialize() const {
        Payload payload;

        uint8_t username_Length = static_cast<uint8_t>(std::min<size_t>(username.size(), 255));
        payload.push_back(username_Length);
        payload.insert(payload.end(), username.begin(), username.begin() + username_Length);
        payload.insert(payload.end(), message.begin(), message.end());

        return payload;
    }

    static Payload serialize(const std::string &username, const std::string &message) {
        Payload payload;

        uint8_t username_length = static_cast<uint8_t>(std::min<size_t>(username.size(), 255));
        payload.push_back(username_length);
        payload.insert(payload.end(), username.begin(), username.begin() + username_length);
        payload.insert(payload.end(), message.begin(), message.end());

        return payload;
    }

    static Message deserialize(const Payload &data) {
        Message msg;

        if (data.empty())
            return msg;

        uint8_t username_length = data[0];
        if (data.size() < 1 + username_length)
            return msg;
        msg.username = std::string(data.begin() + 1, data.begin() + 1 + username_length);
        msg.message  = std::string(data.begin() + 1 + username_length, data.end());

        return msg;
    }
};

inline void send_packet(const int &fd, const Payload payload, Proto::ID packet_id) {
    PacketHeader hdr(packet_id, payload.size(), Proto::Endianness::HOST_TO_NETWORK);
    send(fd, &hdr, sizeof(hdr), 0);
    send(fd, payload.data(), payload.size(), 0);
}

inline void send_packet(const int &fd, const Payload payload, const PacketHeader &hdr) {
    send(fd, &hdr, sizeof(hdr), 0);
    send(fd, payload.data(), payload.size(), 0);
}

/**
 * @brief
 *
 * @param fd
 * @param buffer
 * @return Returns -1 for errors or 0 if the connection closed. Otherwise
 * returns the number of bytes received.
 */
inline int receive_data(const int &fd, Bytes &buffer) {
    Bytes temp(8192);
    int   bytes_received = recv(fd, temp.data(), temp.size(), 0);
    if (bytes_received <= 0) {
        return bytes_received;
    }
    buffer.insert(buffer.end(), temp.begin(), temp.begin() + bytes_received);
    return bytes_received;
}

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

} // namespace Proto