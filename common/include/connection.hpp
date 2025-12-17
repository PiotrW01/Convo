#pragma once
#include "asio.hpp"
#include "asio/ssl.hpp"
#include "asio/system_error.hpp"
#include "client_session.hpp"
#include "packet.hpp"
#include "s_logger.hpp"
#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Proto {
class Connection {

  private:
    asio::ip::basic_resolver_results<asio::ip::tcp> resolve_endpoints(const std::string &ip,
                                                                      const std::string &port) {
        asio::ip::tcp::resolver resolver(m_io_context);
        return resolver.resolve(ip, port);
    }

  public:
    using ReadHandler = std::function<void(const asio::error_code &, std::size_t)>;
    std::shared_ptr<Bytes> read_buffer;
    ClientSession          client_session;
    int                    fd;

  public:
    virtual void async_read(std::shared_ptr<Bytes> buffer, ReadHandler read_handler) = 0;
    virtual void async_write(std::shared_ptr<Bytes> buffer)                          = 0;
    virtual void read(Bytes &buffer)                                                 = 0;
    virtual void write(const Bytes &buffer)                                          = 0;
    virtual bool connect(const std::string &ip, const std::string &port)             = 0;

  protected:
    std::deque<std::shared_ptr<Bytes>> m_write_queue;
    asio::io_context                   m_io_context;
    bool                               m_writing = false;
};

class SSLConnection : public Connection {
  private:
    using SSLStream = asio::ssl::stream<asio::ip::tcp::socket>;
    SSLStream m_ssl_stream;

  public:
    SSLConnection(asio::ip::tcp::socket socket, asio::ssl::context &ctx)
        : m_ssl_stream(std::move(socket), ctx) {
        read_buffer = std::make_shared<Bytes>();
    };

    void read(Bytes &buffer) override {
        Bytes temp(8192);
        int   bytes_received = m_ssl_stream.read_some(asio::buffer(temp));
        if (bytes_received <= 0) {
            // return bytes_received;
        }
        buffer.insert(buffer.end(), temp.begin(), temp.begin() + bytes_received);
        // return bytes_received;
    };

    void write(const Bytes &buffer) override { asio::write(m_ssl_stream, asio::buffer(buffer)); };

    void async_read(std::shared_ptr<Bytes> buffer, ReadHandler read_handler) {
        m_ssl_stream.async_read_some(
            asio::buffer(*buffer, buffer->size()),
            [buffer, read_handler](const asio::error_code &ec, std::size_t bytes) {
                read_handler(ec, bytes);
            });
    };

    void async_write(std::shared_ptr<Bytes> buffer) {
        // m_ssl_stream.async_write_some();
        m_write_queue.push_back(buffer);

        if (m_writing)
            return;

        m_writing = true;
        _write();
    };

    void _write() {
        if (m_write_queue.empty()) {
            m_writing = false;
            return;
        }

        auto buffer = m_write_queue.front();
        asio::async_write(m_ssl_stream, asio::buffer(*buffer),
                          [this](const asio::error_code &ec, std::size_t bytes_transferred) {
                              if (ec) {
                                  Logger::server_message("Write failed: " + ec.message());
                                  m_write_queue.clear();
                                  m_writing = false;
                                  return;
                              }
                              m_write_queue.pop_front();
                              _write();
                          });
    };

    SSLStream &stream() { return m_ssl_stream; }
    bool       handshake(asio::ssl::stream_base::handshake_type type) {
        try {
            m_ssl_stream.handshake(type);
        } catch (asio::system_error &error) {
            Logger::server_message(error.what());
            return false;
        }
        return true;
    };

    bool connect(const std::string &ip, const std::string &port) override {
        asio::ip::tcp::resolver resolver(m_io_context);
        auto                    endpoints = resolver.resolve(ip, port);
        if (endpoints.empty())
            return false;
        try {
            asio::connect(m_ssl_stream.lowest_layer(), endpoints);
        } catch (asio::system_error &error) {
            return false;
        }
        return true;
    };
};

class TCPConnection : public Connection {
  private:
    asio::ip::tcp::socket m_socket;

  public:
    TCPConnection(asio::ip::tcp::socket socket) : m_socket(std::move(socket)) {}

    bool connect(const std::string &ip, const std::string &port) override {};
    void read(Bytes &buffer) override {};
    void write(const Bytes &buffer) override {};

    void async_read(std::shared_ptr<Bytes> buffer, ReadHandler read_handler) override {
        m_socket.async_read_some(
            asio::buffer(*buffer, buffer->size()),
            [buffer, read_handler](const asio::error_code &ec, std::size_t bytes) {
                read_handler(ec, bytes);
            });
    };

    void async_write(std::shared_ptr<Bytes> buffer) override {
        // m_ssl_stream.async_write_some();
        m_write_queue.push_back(buffer);

        if (m_writing)
            return;

        m_writing = true;
        _write();
    };

    void _write() {
        if (m_write_queue.empty()) {
            m_writing = false;
            return;
        }

        auto buffer = m_write_queue.front();
        asio::async_write(m_socket, asio::buffer(*buffer),
                          [this](const asio::error_code &ec, std::size_t bytes_transferred) {
                              if (ec) {
                                  Logger::server_message("Write failed: " + ec.message());
                                  m_write_queue.clear();
                                  m_writing = false;
                                  return;
                              }
                              m_write_queue.pop_front();
                              _write();
                          });
    };
};

} // namespace Proto