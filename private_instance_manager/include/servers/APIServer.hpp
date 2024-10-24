// APIServer.hpp

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
#include <memory>

// Assuming UUID and NetworkInfo are defined appropriately
#include "common/UUID.hpp"
#include "common/NetworkInfo.hpp"

enum class StatusCode {
    OK = 200,
    BadRequest = 400,
    NotFound = 404,
    Gone = 410,
    InternalServerError = 500
};

class APIServer : public std::enable_shared_from_this<APIServer> {
public:
    APIServer(unsigned short port, long timeout_seconds, long cleanup_interval = 10);
    void start();

private:
    // Networking Methods
    void doAccept();
    void handleRequest(boost::asio::ip::tcp::socket socket);
    void scheduleTokenCleanup();

    // Command Processing
    std::string processCommand(const std::string& command);
    std::string addService(std::stringstream& ss);
    std::string getInfo(std::stringstream& ss);

    // Token Management
    void removeExpiredTokens();

    // Helper Methods
    std::string statusMessage(StatusCode code);

    // Attributes
    unsigned short port_;
    long timeout_seconds_;
    long cleanup_interval_;
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::shared_ptr<boost::asio::deadline_timer> cleanup_timer_;

    // Shared map to store tokens and network information
    std::unordered_map<UUID, NetworkInfo> tokens_;
    std::mutex tokens_mutex_;
};

#endif // APISERVER_HPP
