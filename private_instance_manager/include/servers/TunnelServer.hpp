#ifndef TUNNEL_SERVER_HPP
#define TUNNEL_SERVER_HPP

#include <boost/asio.hpp>
#include "clients/APIClient.hpp"


class TunnelServer : public std::enable_shared_from_this<TunnelServer> {

public:
    TunnelServer(unsigned short port, std::string& api_address, unsigned short api_port, unsigned short num_threads = 0);
    void start();
    void stop();

private:
    void doAccept();
    void handleClient(boost::asio::ip::tcp::socket socket);
    void doReadToken(std::shared_ptr<boost::asio::ip::tcp::socket> client_socket, std::shared_ptr<boost::asio::strand<boost::asio::io_context::executor_type>> strand);
    void doResolveInstance(const std::string& token, std::shared_ptr<boost::asio::ip::tcp::socket> client_socket, 
                           std::shared_ptr<boost::asio::strand<boost::asio::io_context::executor_type>> strand);
    void startForwarding(std::shared_ptr<boost::asio::ip::tcp::socket> client_socket,
                         std::shared_ptr<boost::asio::ip::tcp::socket> instance_socket, 
                         std::shared_ptr<boost::asio::strand<boost::asio::io_context::executor_type>> strand);
    
    // Networking components
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::resolver resolver_;
    // Server configuration
    unsigned short port_;
    std::string api_address_;
    unsigned short api_port_;
    // Thread pool
    unsigned short num_threads_;
    std::vector<std::thread> threads_;
};

#endif // TUNNEL_SERVER_HPP