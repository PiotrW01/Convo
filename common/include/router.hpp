#pragma once
#include "connection.hpp"
#include "packet.hpp"
#include <functional>
#include <string>
#include <unordered_map>

namespace Proto {
class Router {
  public:
    Router() = delete;
    Router(asio::ssl::context::method method) : m_ctx(method) {}

    template <typename Packet>
    void on_packet(std::function<void(std::shared_ptr<Proto::Connection>, Packet &)> callback) {
        auto type_id         = typeid(Packet).hash_code();
        m_callbacks[type_id] = [callback](std::shared_ptr<Proto::Connection> conn, void *raw) {
            callback(conn, *static_cast<Packet *>(raw));
        };
    };

    template <typename Packet>
    void dispatch(std::shared_ptr<Proto::Connection> conn, Packet &packet) {
        auto it = m_callbacks.find(typeid(Packet).hash_code());
        if (it != m_callbacks.end()) {
            it->second(conn, &packet);
        }
    }

  protected:
    std::unordered_map<size_t, std::function<void(std::shared_ptr<Proto::Connection>, void *)>>
                       m_callbacks;
    asio::io_context   m_io_context;
    asio::ssl::context m_ctx;
    bool               m_use_ssl = false;

  protected:
    void process_buffer(std::shared_ptr<Proto::Connection> conn, Proto::Bytes &read_buffer) {
        while (Proto::is_header_ready(read_buffer)) {
            Proto::PacketHeader hdr(read_buffer, Proto::Endianness::NETWORK_TO_HOST);
            Proto::PayloadSize  payload_size = hdr.payload_size;
            if (!Proto::is_packet_ready(read_buffer, payload_size))
                break;

            Proto::Payload payload = Proto::get_payload(read_buffer, payload_size);

            switch (hdr.id) {
            case Proto::PACKET_ID::LOGIN: {
                Proto::Login req;
                req.deserialize(payload);
                dispatch(conn, req);
                break;
            }
            case Proto::PACKET_ID::MESSAGE: {
                Proto::Message req;
                req.deserialize(payload);
                dispatch(conn, req);
                break;
            }
            case Proto::PACKET_ID::REGISTER: {
                Proto::Register req;
                req.deserialize(payload);
                dispatch(conn, req);
            }
            default:
                break;
            };
            Proto::remove_packet(read_buffer, payload_size);
        }
    };
};

class ClientRouter : public Router {

  private:
    std::shared_ptr<Proto::Connection> m_server;

  public:
    ClientRouter() : Router(asio::ssl::context::tls_client) {}

    void use_ssl() {
        m_ctx.set_default_verify_paths();
        m_ctx.set_verify_mode(asio::ssl::context_base::verify_none);
        m_use_ssl = true;
    };

    void
    connect_and_run(const std::string &ip, const std::string &port,
                    std::function<void(ClientSession::Status connection_status)> connection_cb) {
        auto socket = std::make_shared<asio::ip::tcp::socket>(m_io_context);

        if (m_use_ssl) {
            auto ssl_conn = std::make_shared<SSLConnection>(std::move(*socket), m_ctx);
            if (ssl_conn->connect(ip, port)) {
                ssl_conn->stream().async_handshake(
                    asio::ssl::stream_base::client,
                    [this, ssl_conn, connection_cb](const asio::error_code &handshake_ec) {
                        if (!handshake_ec) {
                            connection_cb(ClientSession::Status::CONNECTED);
                            int fd       = ssl_conn->stream().next_layer().native_handle();
                            ssl_conn->fd = fd;
                            client_loop(ssl_conn);
                        } else {
                            connection_cb(ClientSession::Status::HANDSHAKE_FAILED);
                        }
                    });

                m_server = ssl_conn;
            } else {
                connection_cb(ClientSession::Status::CONNECTION_FAILED);
            }
        } else {
            m_server = std::make_unique<TCPConnection>(std::move(*socket));
            if (m_server->connect(ip, port)) {
                connection_cb(ClientSession::Status::CONNECTED);
                client_loop(m_server);
            } else {
                connection_cb(ClientSession::Status::CONNECTION_FAILED);
            }
        }
        m_io_context.run();
    };

    void client_loop(std::shared_ptr<Proto::Connection> conn) {
        auto temp         = std::make_shared<Proto::Bytes>(8192);
        auto read_handler = [this, conn, temp](const asio::error_code &ec, std::size_t bytes_recv) {
            if (!ec && bytes_recv > 0) {
                auto read_buffer = conn->read_buffer;
                read_buffer->insert(read_buffer->end(), temp->begin(), temp->begin() + bytes_recv);
                process_buffer(conn, *read_buffer);
                client_loop(conn);
            } else {
                // m_io_context.shutdown();
            }
        };
        conn->async_read(temp, read_handler);
    };

    void send_packet(Proto::Packet &packet) {
        std::shared_ptr<Bytes> s = std::make_shared<Bytes>(packet.serialize());
        m_server->async_write(s);
    };
};

class ServerRouter : public Router {
  private:
    asio::ip::tcp::acceptor m_acceptor;

  private:
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
                Logger::server_message("Client disconnected " + ec.message());
            }
        };

        conn->async_read(temp, read_handler);
    };

  public:
    std::unordered_map<int, std::shared_ptr<Proto::Connection>> clients;

  public:
    ServerRouter(uint16_t port)
        : Router(asio::ssl::context::tls_server),
          m_acceptor(m_io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {}

    void use_ssl(const std::string &cert, const std::string &key) {
        m_ctx.use_certificate_chain_file(cert);
        m_ctx.use_private_key_file(key, asio::ssl::context::pem);
        //! m_ctx.use_private_key_file(cert, asio::ssl::context::pem);  switch to this instead
        //! so only need to keep track of the pem file
        m_ctx.set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 |
                          asio::ssl::context::no_sslv3 | asio::ssl::context::no_tlsv1 |
                          asio::ssl::context::no_tlsv1_1);

        m_use_ssl = true;
    };

    void send_packet(std::shared_ptr<Proto::Connection> conn, Proto::Packet &packet) {
        std::shared_ptr<Bytes> s = std::make_shared<Bytes>(packet.serialize());
        conn->async_write(s);
    };

    void run() {
        accept_loop();
        m_io_context.run();
    }
};
} // namespace Proto
