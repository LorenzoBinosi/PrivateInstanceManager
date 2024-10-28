#ifndef INSTANCESSERVER_HPP
#define INSTANCESSERVER_HPP

#include <boost/asio.hpp>
#include <string>
#include "clients/APIClient.hpp"

class InstancesServer {
public:
    // Constructor to initialize the server with a specified port
    InstancesServer(unsigned long port, std::string api_endpoint, unsigned long api_port, long timeout, 
        std::string command, std::string challenge_endpoint, std::string challenge_port,
        bool ssl);
    // Method to start the server
    void start();

private:
    // Method to handle client connections
    void handleClient(boost::asio::ip::tcp::socket client_socket);

    // Attributes
    unsigned long port_;
    std::string api_endpoint_;
    unsigned long api_port_;
    long timeout_;
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::string command_;
    std::string challenge_endpoint_;
    std::string challenge_port_;
    bool ssl_;
    APIClient client_;
};

#endif // INSTANCESSERVER_HPP