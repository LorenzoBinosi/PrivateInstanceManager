#ifndef REGISTERINGSERVER_HPP
#define REGISTERINGSERVER_HPP

#include <boost/asio.hpp>

class RegisteringServer {
public:
    // Constructor to initialize the server with a specified port
    RegisteringServer(unsigned long port, unsigned long api_port, long timeout);
    // Method to start the server
    void start();

private:
    // Method to handle client connections
    void handleClient(boost::asio::ip::tcp::socket socket);

    // Attributes
    unsigned long port;
    unsigned long api_port;
    long timeout;
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor;
};

#endif // REGISTERINGSERVER_HPP