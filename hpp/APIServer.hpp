#ifndef APISERVER_HPP
#define APISERVER_HPP

#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <unordered_map>
#include <mutex>
#include <string>
#include "UUID.hpp"
#include "NetworkInfo.hpp"

class APIServer : public std::enable_shared_from_this<APIServer>{
public:
    APIServer(unsigned long port, long timeout);
    void start();

private:
    // Methods
    void runServer();
    void doAccept();
    void handleRequest(boost::asio::ip::tcp::socket socket);
    std::string processCommand(const std::string& command);
    void removeExpiredTokens();
    // API Methods
    std::string addService(std::stringstream& ss);
    std::string getInfo(std::stringstream& ss);

    // Attributes
    unsigned long port;
    long timeout;
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor;
    // Shared map to store tokens and network information
    std::unordered_map<UUID, NetworkInfo> tokens;
    std::mutex tokens_mutex;
};

#endif // APISERVER_HPP
