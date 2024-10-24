#ifndef TUNNEL_SERVER_HPP
#define TUNNEL_SERVER_HPP

#include <boost/asio.hpp>
#include "clients/APIClient.hpp"


class TunnelServer : public std::enable_shared_from_this<TunnelServer> {

public:
    TunnelServer(boost::asio::ip::tcp::socket socket, std::string api_endpoint, unsigned long api_port, std::string instance_endpoint);
    void start();

private:
    void doReadToken();
    void doResolveInstance();
    void startForwarding(boost::asio::ip::tcp::socket& from_socket, boost::asio::ip::tcp::socket& to_socket);
    
    
    boost::asio::ip::tcp::socket client_socket_;
    boost::asio::ip::tcp::socket instance_socket_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::strand<boost::asio::any_io_executor> strand_;
    boost::asio::streambuf token_buffer_;
    std::string token_;
    APIClient client_;
    std::string api_endpoint_;
    unsigned long api_port_;
    std::string instance_endpoint_;
    unsigned long instance_port_;
};

#endif // TUNNEL_SERVER_HPP