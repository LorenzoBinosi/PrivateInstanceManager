#include "TunnelServer.hpp"

#include <iostream>
#include <boost/asio.hpp>
#include "API.hpp"



TunnelServer::TunnelServer(boost::asio::ip::tcp::socket socket, unsigned long api_port, std::string instance_endpoint)
    : client_socket(std::move(socket)),
        instance_socket(client_socket.get_executor()),
        resolver(client_socket.get_executor()),
        strand(boost::asio::make_strand(client_socket.get_executor())),
        api_port(api_port),
        instance_endpoint(std::move(instance_endpoint)) {
}

void TunnelServer::start() {
    auto self(shared_from_this());
    boost::asio::post(strand, [this, self]() {
        doReadToken();
    });
}

void TunnelServer::doReadToken() {
    auto self(shared_from_this());
    boost::asio::async_write(client_socket, boost::asio::buffer("Token: "),
        boost::asio::bind_executor(strand,
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    boost::asio::async_read_until(client_socket, token_buffer, '\n',
                        boost::asio::bind_executor(strand,
                            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                                if (!ec) {
                                    std::istream is(&token_buffer);
                                    std::getline(is, token);
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
    instance_port = apiGetInfoPort(api_port, token);
    if (instance_port == (unsigned long)-1) {
        auto self(shared_from_this());
        boost::asio::async_write(client_socket, boost::asio::buffer("Invalid token\n"),
            boost::asio::bind_executor(strand,
                [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                    if (ec) {
                        std::cerr << "Write invalid token error: " << ec.message() << std::endl;
                    }
                }));
        return;
    }
    auto self(shared_from_this());
    boost::asio::async_write(client_socket, boost::asio::buffer("Token is correct. Connecting to private instance...\n"),
        boost::asio::bind_executor(strand,
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // Resolve and connect to the instance
                    resolver.async_resolve(instance_endpoint, std::to_string(instance_port),
                        boost::asio::bind_executor(strand,
                            [this, self](boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type endpoints) {
                                if (!ec) {
                                    boost::asio::async_connect(instance_socket, endpoints,
                                        boost::asio::bind_executor(strand,
                                            [this, self](boost::system::error_code ec, const boost::asio::ip::tcp::endpoint& /*endpoint*/) {
                                                if (!ec) {
                                                    // Start forwarding data
                                                    startForwarding(client_socket, instance_socket);
                                                    startForwarding(instance_socket, client_socket);
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
        boost::asio::bind_executor(strand,
            [this, self, buffer, &from_socket, &to_socket](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (!ec && bytes_transferred > 0) {
                    boost::asio::async_write(to_socket, boost::asio::buffer(*buffer, bytes_transferred),
                        boost::asio::bind_executor(strand,
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

