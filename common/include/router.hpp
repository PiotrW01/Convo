#pragma once
#include "connection.hpp"
#include "packet.hpp"
#include <functional>
#include <string>
#include <unordered_map>

namespace Proto {
class Router {

  public:
    virtual void run() = 0;

    template <typename Packet> void on_packet(std::function<void(int, const Packet &)> callback) {
        auto type_id         = typeid(Packet).hash_code();
        m_callbacks[type_id] = [callback](int fd, const void *raw) {
            callback(fd, *static_cast<const Packet *>(raw));
        };
    };

    template <typename Packet> void dispatch(int fd, const Packet &packet) {
        auto it = m_callbacks.find(typeid(Packet).hash_code());
        if (it != m_callbacks.end()) {
            it->second(fd, &packet);
        }
    }

  protected:
    std::unordered_map<size_t, std::function<void(int, const void *)>> m_callbacks;
};

class ClientRouter : public Router {

  private:
    std::unique_ptr<Proto::Connection> m_connection;
    bool                               m_use_ssl = false;

  public:
    void use_ssl() { m_use_ssl = true; };
    bool connect(const std::string &ip, const std::string &port) {
        if (m_use_ssl) {
            // asio::io_context io_context;
            // m_connection = std::make_unique<SSLConnection>(io_context);
        } else {
            // m_connection = std::make_unique<TCPConnection>();
        }
        bool is_connected = m_connection->init(ip, port);
        return is_connected;
    };
    void run() override {
        Proto::Bytes buffer;

        while (true) {
            int bytes = 0;
            if (bytes <= 0)
                break;

            while (Proto::is_header_ready(buffer)) {
                Proto::PacketHeader hdr(buffer, Proto::Endianness::NETWORK_TO_HOST);
                Proto::PayloadSize  payload_size = hdr.payload_size;
                if (!Proto::is_packet_ready(buffer, payload_size))
                    break;

                Proto::Payload payload = Proto::get_payload(buffer, payload_size);

                switch (hdr.id) {
                case Proto::PACKET_ID::LOGIN: {
                    Proto::LoginRequest req;
                    req.deserialize(payload);
                    dispatch(0, req);
                    break;
                }
                case Proto::PACKET_ID::MESSAGE: {
                    Proto::Message req;
                    req.deserialize(payload);
                    dispatch(0, req);
                    break;
                }
                default:
                    break;
                };
                Proto::remove_packet(buffer, payload_size);
            }
        }
    };

    void send_packet(Proto::Packet &packet) { m_connection->write(packet.serialize()); };
};

class ServerRouter : public Router {
  private:
    asio::io_context        m_io_context;
    asio::ip::tcp::acceptor m_acceptor;
    asio::ssl::context      m_ctx;
    bool                    m_use_ssl = false;

  public:
    std::unordered_map<int, std::shared_ptr<Proto::Connection>> clients;
    ServerRouter(uint16_t port)
        : m_acceptor(m_io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
          m_ctx(asio::ssl::context::tls_server) {}
    void use_ssl(const std::string &cert, const std::string &key) {
        m_ctx.use_certificate_chain_file(cert);
        m_ctx.use_private_key_file(key, asio::ssl::context::pem);
        m_ctx.set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 |
                          asio::ssl::context::no_sslv3 | asio::ssl::context::no_tlsv1 |
                          asio::ssl::context::no_tlsv1_1);

        m_use_ssl = true;
    };

    void accept_loop() {
        auto socket = std::make_shared<asio::ip::tcp::socket>(m_io_context);

        m_acceptor.async_accept(*socket, [this, socket](const asio::error_code &ec) {
            if (!ec) {
                std::shared_ptr<Proto::Connection> conn;

                if (m_use_ssl) {
                    auto ssl_conn =
                        std::make_shared<Proto::SSLConnection>(std::move(*socket), m_ctx);

                    ssl_conn->stream().async_handshake(
                        asio::ssl::stream_base::server,
                        [this, ssl_conn](const asio::error_code &handshake_ec) {
                            if (!handshake_ec) {
                                int fd       = ssl_conn->stream().next_layer().native_handle();
                                ssl_conn->fd = fd;
                                clients[fd]  = ssl_conn;
                                client_loop(ssl_conn);
                            } else {
                                Logger::server_message("SSL handshake failed: " +
                                                       handshake_ec.message());
                            }
                        });

                    conn = ssl_conn;
                } else {
                    conn        = std::make_shared<Proto::TCPConnection>(std::move(*socket));
                    int fd      = socket->native_handle();
                    conn->fd    = fd;
                    clients[fd] = conn;
                    client_loop(conn);
                }
            }
            accept_loop();
        });
    }

    void client_loop(std::shared_ptr<Proto::Connection> conn) {
        auto temp         = std::make_shared<Proto::Bytes>(8192);
        auto read_handler = [this, conn, temp](const asio::error_code &ec, std::size_t bytes_recv) {
            if (!ec && bytes_recv > 0) {
                auto client_read_buffer = conn->read_buffer;
                client_read_buffer->insert(client_read_buffer->end(), temp->begin(),
                                           temp->begin() + bytes_recv);
                process_buffer(conn, *client_read_buffer);
                client_loop(conn);
            } else {
                clients.erase(conn->fd);
                Logger::server_message("Client disconnected");
            }
        };

        conn->async_read(temp, read_handler);
    };

    void process_buffer(std::shared_ptr<Proto::Connection> conn, Proto::Bytes &client_read_buffer) {
        while (Proto::is_header_ready(client_read_buffer)) {
            Proto::PacketHeader hdr(client_read_buffer, Proto::Endianness::NETWORK_TO_HOST);
            Proto::PayloadSize  payload_size = hdr.payload_size;
            if (!Proto::is_packet_ready(client_read_buffer, payload_size))
                break;

            Proto::Payload payload = Proto::get_payload(client_read_buffer, payload_size);

            switch (hdr.id) {
            case Proto::PACKET_ID::LOGIN: {
                Proto::LoginRequest req;
                req.deserialize(payload);
                dispatch(0, req);
                break;
            }
            case Proto::PACKET_ID::MESSAGE: {
                Proto::Message req;
                req.deserialize(payload);
                dispatch(0, req);
                break;
            }
            default:
                break;
            };
            Proto::remove_packet(client_read_buffer, payload_size);
        }
    };
    void run() override {
        accept_loop();
        m_io_context.run();
    }
};
} // namespace Proto
