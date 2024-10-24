#ifndef APICLIENT_HPP
#define APICLIENT_HPP

#include <string>
#include <boost/asio.hpp>


class APIClient {
public:
    APIClient(const std::string& endpoint, unsigned long api_port);
    ~APIClient();

    bool apiPing();
    unsigned long apiGetInfoPort(const std::string& uuid_str);
    std::string apiAddService(unsigned long service_port);

private:
    std::string sendRequest(const std::string& message);
    void connect();
    void disconnect();

    std::string endpoint_;
    unsigned long api_port_;
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::socket socket_;
};

#endif // APICLIENT_HPP