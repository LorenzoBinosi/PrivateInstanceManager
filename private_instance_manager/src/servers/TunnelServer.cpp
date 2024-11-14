#include "servers/TunnelServer.hpp"
#include <iostream>
#include <boost/asio.hpp>


static APIClient& get_thread_api_client(const std::string& api_endpoint, unsigned short api_port) {
    thread_local std::unique_ptr<APIClient> api_client = nullptr;
    if (!api_client) {
        api_client = std::make_unique<APIClient>(api_endpoint, api_port);
    }
    return *api_client;
}

TunnelServer::TunnelServer(unsigned short port, std::string& api_address, unsigned short api_port, unsigned short num_threads)
    : io_context_(),
      acceptor_(io_context_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      resolver_(io_context_),
      port_(port),
      api_address_(api_address),
      api_port_(api_port),
      num_threads_(num_threads) {
    if (num_threads_ == 0) {
        num_threads_ = std::thread::hardware_concurrency();
        if (num_threads_ == 0) {
            num_threads_ = 1;
        }
    }
}

void TunnelServer::start() {
    std::cout << "Server started." << std::endl;
    std::cout << "Using " << num_threads_ << " threads." << std::endl;
    std::cout << "Waiting for incoming connections on port " << port_ << "." << std::endl;
    doAccept();
    // Start the thread pool
    for (unsigned int i = 0; i < num_threads_; ++i) {
        threads_.emplace_back([this]() {
            io_context_.run();
        });
    }}

void TunnelServer::stop() {
    io_context_.stop();
    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void TunnelServer::doAccept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                // Handle the client connection
                handleClient(std::move(socket));
            } else {
                std::cerr << "Accept error: " << ec.message() << std::endl;
                socket.close();
            }
            // Continue accepting new connections
            doAccept();
        });
}

void TunnelServer::handleClient(boost::asio::ip::tcp::socket socket) {
    // Creating a new shared pointer for the socket
    auto client_socket = std::make_shared<boost::asio::ip::tcp::socket>(std::move(socket));
    // Creating a new strand for this connection (Is this correct?)
    auto strand = std::make_shared<boost::asio::strand<boost::asio::io_context::executor_type>>(boost::asio::make_strand(io_context_));
    doReadToken(client_socket, strand);
}

void TunnelServer::doReadToken(std::shared_ptr<boost::asio::ip::tcp::socket> client_socket, std::shared_ptr<boost::asio::strand<boost::asio::io_context::executor_type>> strand) {
    auto self(shared_from_this());
        
    boost::asio::async_write(*client_socket, boost::asio::buffer("Token: "),
        boost::asio::bind_executor(*strand,
            [this, self, client_socket, strand](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    auto token_buffer = std::make_shared<boost::asio::streambuf>();
                    boost::asio::async_read_until(*client_socket, *token_buffer, '\n',
                        boost::asio::bind_executor(*strand,
                            [this, self, token_buffer, client_socket, strand](boost::system::error_code ec, std::size_t /*length*/) {
                                if (!ec) {
                                    std::istream is(token_buffer.get());
                                    std::string token;
                                    std::getline(is, token);
                                    doResolveInstance(token, client_socket, strand);
                                } else {
                                    std::cerr << "Read token error: " << ec.message() << std::endl;
                                    client_socket->close();
                                }
                            }));
                } else {
                    std::cerr << "Write token prompt error: " << ec.message() << std::endl;
                    client_socket->close();
                }
            }));
}



void TunnelServer::doResolveInstance(const std::string& token, std::shared_ptr<boost::asio::ip::tcp::socket> client_socket, 
                                     std::shared_ptr<boost::asio::strand<boost::asio::io_context::executor_type>> strand) {
    // Retrieve the instance port using the token
    unsigned long port;
    long time_remaining;
    std::shared_ptr<std::string> address;

    auto self(shared_from_this());
    APIClient& client = get_thread_api_client(api_address_, api_port_);
    auto result = client.apiGetInfo(token);
    if (result)
    {
        port = std::get<0>(*result);
        time_remaining = std::get<1>(*result);
        address = std::make_shared<std::string>(std::get<2>(*result));
        if (port == 0) {
            boost::asio::async_write(*client_socket, boost::asio::buffer("Invalid Token!\n"),
                boost::asio::bind_executor(*strand,
                    [this, self, client_socket](boost::system::error_code ec, std::size_t /*length*/) {
                        if (ec) {
                            std::cerr << "[Write error] \"Invalid Token!\": " << ec.message() << std::endl;
                            client_socket->close();
                        }
                    }));
            return;
        }
        if (time_remaining <= 0) {
            boost::asio::async_write(*client_socket, boost::asio::buffer("Token has expired!\n"),
                boost::asio::bind_executor(*strand,
                    [this, self, client_socket](boost::system::error_code ec, std::size_t /*length*/) {
                        if (ec) {
                            std::cerr << "[Write error] \"Token has expired!\": " << ec.message() << std::endl;
                            client_socket->close();
                        }
                    }));
            return;
        }
    }
    else
    {
        boost::asio::async_write(*client_socket, boost::asio::buffer("Invalid request!\n"),
            boost::asio::bind_executor(*strand,
                [this, self, client_socket](boost::system::error_code ec, std::size_t /*length*/) {
                    if (ec) {
                        std::cerr << "[Write error] \"Invalid request!\": " << ec.message() << std::endl;
                        client_socket->close();
                    }
                }));
        return;
    }
    boost::asio::async_write(*client_socket, boost::asio::buffer("Token is correct. Connecting to private instance...\n"),
        boost::asio::bind_executor(*strand,
            [this, self, address, port, client_socket, strand](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // Resolve and connect to the instance
                    resolver_.async_resolve(*address, std::to_string(port),
                        boost::asio::bind_executor(*strand,
                            [this, self, client_socket, address, strand](boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type endpoints) {
                                if (!ec) {
                                    auto instance_socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context_);
                                    boost::asio::async_connect(*instance_socket, endpoints,
                                        boost::asio::bind_executor(*strand,
                                            [this, self, client_socket, instance_socket, strand](boost::system::error_code ec, const boost::asio::ip::tcp::endpoint& /*endpoint*/) {
                                                if (!ec) {
                                                    // Start forwarding data
                                                    startForwarding(client_socket, instance_socket, strand);
                                                    startForwarding(instance_socket, client_socket, strand);
                                                } else {
                                                    std::cerr << "Connect to instance error: " << ec.message() << std::endl;
                                                    client_socket->close();
                                                    instance_socket->close();
                                                }
                                            }));
                                } else {
                                    std::cerr << "Resolve instance error: " << ec.message() << std::endl;
                                    client_socket->close();
                                }
                            }));
                } else {
                    std::cerr << "Write confirmation error: " << ec.message() << std::endl;
                    client_socket->close();
                }
            }));
}

void TunnelServer::startForwarding(std::shared_ptr<boost::asio::ip::tcp::socket> from_socket,
                                   std::shared_ptr<boost::asio::ip::tcp::socket> to_socket, 
                                   std::shared_ptr<boost::asio::strand<boost::asio::io_context::executor_type>> strand) {
    auto buffer = std::make_shared<std::vector<char>>(8192);
    auto self(shared_from_this());
    from_socket->async_read_some(boost::asio::buffer(*buffer),
        boost::asio::bind_executor(*strand,
            [this, self, buffer, from_socket, to_socket, strand](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (!ec && bytes_transferred > 0) {
                    boost::asio::async_write(*to_socket, boost::asio::buffer(*buffer, bytes_transferred),
                        boost::asio::bind_executor(*strand,
                            [this, self, buffer, from_socket, to_socket, strand](boost::system::error_code write_ec, std::size_t /*bytes_written*/) {
                                if (!write_ec) {
                                    startForwarding(from_socket, to_socket, strand);
                                } else {
                                    std::cerr << "Write error: " << write_ec.message() << std::endl;
                                    from_socket->close();
                                    to_socket->close();
                                }
                            }));
                } else if (ec != boost::asio::error::operation_aborted) {
                    std::cerr << "Read error: " << ec.message() << std::endl;
                    from_socket->close();
                    to_socket->close();
                }
            }));
}

