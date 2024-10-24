#include "servers/TunnelServer.hpp"
#include <iostream>
#include <boost/asio.hpp>


TunnelServer::TunnelServer(boost::asio::ip::tcp::socket socket, std::string api_endpoint, unsigned long api_port, std::string instance_endpoint)
    : client_socket_(std::move(socket)),
        instance_socket_(client_socket_.get_executor()),
        resolver_(client_socket_.get_executor()),
        strand_(boost::asio::make_strand(client_socket_.get_executor())),
        client_(api_endpoint, api_port),
        api_endpoint_(std::move(api_endpoint)),
        api_port_(api_port),
        instance_endpoint_(std::move(instance_endpoint)){
}

void TunnelServer::start() {
    auto self(shared_from_this());
    boost::asio::post(strand_, [this, self]() {
        doReadToken();
    });
}

void TunnelServer::doReadToken() {
    auto self(shared_from_this());
    boost::asio::async_write(client_socket_, boost::asio::buffer("Token: "),
        boost::asio::bind_executor(strand_,
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    boost::asio::async_read_until(client_socket_, token_buffer_, '\n',
                        boost::asio::bind_executor(strand_,
                            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                                if (!ec) {
                                    std::istream is(&token_buffer_);
                                    std::getline(is, token_);
                                    doResolveInstance();
                                } else {
                                    std::cerr << "Read token error: " << ec.message() << std::endl;
                                }
                            }));
                } else {
                    std::cerr << "Write token prompt error: " << ec.message() << std::endl;
                }
            }));
}

void TunnelServer::doResolveInstance() {
    // Retrieve the instance port using the token
    instance_port_ = client_.apiGetInfoPort(token_);
    if (instance_port_ == (unsigned long)-1) {
        auto self(shared_from_this());
        boost::asio::async_write(client_socket_, boost::asio::buffer("Invalid token\n"),
            boost::asio::bind_executor(strand_,
                [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                    if (ec) {
                        std::cerr << "Write invalid token error: " << ec.message() << std::endl;
                    }
                }));
        return;
    }
    auto self(shared_from_this());
    boost::asio::async_write(client_socket_, boost::asio::buffer("Token is correct. Connecting to private instance...\n"),
        boost::asio::bind_executor(strand_,
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // Resolve and connect to the instance
                    resolver_.async_resolve(instance_endpoint_, std::to_string(instance_port_),
                        boost::asio::bind_executor(strand_,
                            [this, self](boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type endpoints) {
                                if (!ec) {
                                    boost::asio::async_connect(instance_socket_, endpoints,
                                        boost::asio::bind_executor(strand_,
                                            [this, self](boost::system::error_code ec, const boost::asio::ip::tcp::endpoint& /*endpoint*/) {
                                                if (!ec) {
                                                    // Start forwarding data
                                                    startForwarding(client_socket_, instance_socket_);
                                                    startForwarding(instance_socket_, client_socket_);
                                                } else {
                                                    std::cerr << "Connect to instance error: " << ec.message() << std::endl;
                                                }
                                            }));
                                } else {
                                    std::cerr << "Resolve instance error: " << ec.message() << std::endl;
                                }
                            }));
                } else {
                    std::cerr << "Write confirmation error: " << ec.message() << std::endl;
                }
            }));
}

void TunnelServer::startForwarding(boost::asio::ip::tcp::socket& from_socket, boost::asio::ip::tcp::socket& to_socket) {
    auto buffer = std::make_shared<std::vector<char>>(8192);
    auto self(shared_from_this());
    from_socket.async_read_some(boost::asio::buffer(*buffer),
        boost::asio::bind_executor(strand_,
            [this, self, buffer, &from_socket, &to_socket](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (!ec && bytes_transferred > 0) {
                    boost::asio::async_write(to_socket, boost::asio::buffer(*buffer, bytes_transferred),
                        boost::asio::bind_executor(strand_,
                            [this, self, buffer, &from_socket, &to_socket](boost::system::error_code write_ec, std::size_t /*bytes_written*/) {
                                if (!write_ec) {
                                    startForwarding(from_socket, to_socket);
                                } else {
                                    std::cerr << "Write error: " << write_ec.message() << std::endl;
                                    from_socket.close();
                                    to_socket.close();
                                }
                            }));
                } else if (ec != boost::asio::error::operation_aborted) {
                    std::cerr << "Read error: " << ec.message() << std::endl;
                    from_socket.close();
                    to_socket.close();
                }
            }));
}

