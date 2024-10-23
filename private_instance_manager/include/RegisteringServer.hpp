#ifndef REGISTERINGSERVER_HPP
#define REGISTERINGSERVER_HPP

#include <boost/asio.hpp>

class RegisteringServer {
public:
    // Constructor to initialize the server with a specified port
    RegisteringServer(unsigned long port, std::string api_endpoint, unsigned long api_port, long timeout, 
        std::string command, std::string challenge_endpoint, std::string challenge_port,
        bool ssl);
    // Method to start the server
    void start();

private:
    // Method to handle client connections
    void handleClient(boost::asio::ip::tcp::socket client_socket);

    // Attributes
    unsigned long port;
    std::string api_endpoint;
    unsigned long api_port;
    long timeout;
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor;
    std::string command;
    std::string challenge_endpoint;
    std::string challenge_port;
    bool ssl;
};

#endif // REGISTERINGSERVER_HPP