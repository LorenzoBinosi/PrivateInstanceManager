#ifndef TUNNEL_SERVER_HPP
#define TUNNEL_SERVER_HPP

#include <boost/asio.hpp>


class TunnelServer : public std::enable_shared_from_this<TunnelServer> {

public:
    TunnelServer(boost::asio::ip::tcp::socket socket, unsigned long api_port, std::string instance_endpoint);
    void start();

private:
    void doReadToken();
    void doResolveInstance();
    void startForwarding(boost::asio::ip::tcp::socket& from_socket, boost::asio::ip::tcp::socket& to_socket);
    
    boost::asio::ip::tcp::socket client_socket;
    boost::asio::ip::tcp::socket instance_socket;
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::strand<boost::asio::any_io_executor> strand;
    boost::asio::streambuf token_buffer;
    std::string token;
    unsigned long api_port;
    std::string instance_endpoint;
    unsigned long instance_port;
};

#endif // TUNNEL_SERVER_HPP