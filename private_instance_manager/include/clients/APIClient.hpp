#ifndef APICLIENT_HPP
#define APICLIENT_HPP

#include <string>
#include <boost/asio.hpp>
#include <optional>
#include "common/NetworkInfo.hpp"


class APIClient {
public:
    APIClient(const std::string& api_address, unsigned short api_port);
    ~APIClient();

    bool apiPing();
    std::optional<std::tuple<unsigned short, long, std::string>> apiGetInfo(const std::string& uuid_str);
    std::optional<std::string> apiAddService(const std::string& service_address, unsigned short service_port);

private:
    std::string sendRequest(const std::string& message);
    void connect();
    void disconnect();

    std::string api_address_;
    unsigned short api_port_;
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::socket socket_;
};

#endif // APICLIENT_HPP