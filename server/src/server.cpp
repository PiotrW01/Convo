#include "server.hpp"
#include "logger.hpp"
#include <format>
#include <iostream>

Server::Server(std::optional<int> port) {
    // std::signal(SIGINT, stop);
    // std::signal(SIGHUP, stop);
    // std::signal(SIGTERM, stop);

    m_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_server_socket == -1) {
        Logger::error("\nCould not create server socket.\n");
        exit(-1);
    }

    sockaddr_in server_address;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port        = htons(m_server_port);
    server_address.sin_family      = AF_INET;

    int yes = 1;
    setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    bind(m_server_socket, reinterpret_cast<struct sockaddr *>(&server_address),
         sizeof(server_address));
    int status = listen(m_server_socket, 128);
    if (status == -1) {
        Logger::error("\nCould not start listening on server socket.\n");
        exit(-1);
    }

    m_server_poll.fd     = m_server_socket;
    m_server_poll.events = POLLIN;
    m_fds.push_back(m_server_poll);
}

void Server::run() {
    Logger::server_message(std::format("Server is running on port {}", m_server_port));
    Logger::info(std::format("Server is running on port {}", m_server_port));
    Logger::warn(std::format("Server is running on port {}", m_server_port));
    Logger::error(std::format("Server is running on port {}", m_server_port));
    Logger::client_message("glob", std::format("Server is running on port {}", m_server_port));
    while (m_running) {
        int events = poll(m_fds.data(), m_fds.size(), 1000);
        if (events < 0) {
            perror("poll");
            break;
        }
        handle_connections(m_fds);
    }
}

void Server::broadcast_message(const Proto::Message &msg) {
    Proto::Payload      payload = msg.serialize();
    Proto::PacketHeader hdr(Proto::ID::MESSAGE, payload.size(), Proto::Endianness::HOST_TO_NETWORK);

    for (auto &client : m_fds) {
        if (client.fd == m_server_socket)
            continue;
        Proto::send_packet(client.fd, payload, hdr);
    }
}

void Server::handle_connections(std::vector<pollfd> &m_fds) {
    for (int it = 0; it < m_fds.size(); it++) {
        if (!(m_fds[it].revents & POLLIN))
            continue;

        if (m_fds[it].fd == m_server_socket) {
            int fd = accept(m_server_socket, nullptr, nullptr);
            if (fd >= 0) {
                connect_client(fd);
            }
            continue;
        }

        auto &client_buffer  = m_client_sessions[m_fds[it].fd].buffer;
        int   bytes_received = Proto::receive_data(m_fds[it].fd, client_buffer);
        if (bytes_received <= 0) {
            disconnect_client(it);
            continue;
        }
        try_process_data(m_fds[it].fd, client_buffer);
    }
}

void Server::try_process_data(int fd, Proto::Bytes &buffer) {
    while (Proto::is_header_ready(buffer)) {
        Proto::PacketHeader hdr(buffer, Proto::Endianness::NETWORK_TO_HOST);
        Proto::PayloadSize  payload_size = hdr.length;

        if (!Proto::is_packet_ready(buffer, payload_size))
            break;

        Proto::Payload payload = Proto::get_payload(buffer, payload_size);
        switch (hdr.id) {
        case Proto::ID::MESSAGE: {
            Proto::Message msg = Proto::Message::deserialize(payload);
            msg.username       = m_client_sessions[fd].username;
            Logger::client_message(msg.username, msg.message);
            broadcast_message(msg);
            break;
        }
        case Proto::ID::LOGIN: {
            Proto::LoginRequest req = Proto::LoginRequest::deserialize(payload);

            auto &client = m_client_sessions[fd];

            if (req.username == "" || client.status == ClientSession::LOGGED_IN)
                break;
            client.username = req.username;
            client.status   = ClientSession::LOGGED_IN;

            Proto::Message msg;
            msg.username = "Server";
            msg.message  = std::format("{} joined the channel.", req.username);

            Proto::send_packet(fd, payload, Proto::ID::LOGIN);
            Logger::server_message(msg.message);
            broadcast_message(msg);
            break;
        }
        default:
            break;
        }

        Proto::remove_packet(buffer, payload_size);
    }
}

void Server::disconnect_client(int &it) {
    std::string msg_disconnect(
        std::format("{} left the channel.", m_client_sessions[m_fds[it].fd].username));
    close(m_fds[it].fd);
    m_client_sessions.erase(m_fds[it].fd);
    m_fds.erase(m_fds.begin() + it);

    // we reduce it by 1 so the handleConnections loop doesn't skip over the next client in the
    // shortened m_fds array
    it--;

    Proto::Message msg;
    msg.username = "Server";
    msg.message  = msg_disconnect;

    Logger::server_message(msg_disconnect);
    broadcast_message(msg);
}

void Server::connect_client(int fd) {
    pollfd client_poll{};
    client_poll.fd     = fd;
    client_poll.events = POLLIN;
    m_fds.push_back(client_poll);
    ClientSession session;
    session.status        = ClientSession::CONNECTED;
    m_client_sessions[fd] = session;

    Logger::info(std::format("Client connected with id: {}", fd));
}
