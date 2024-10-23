#include "APIServer.hpp"
#include <iostream>
#include <sstream>
#include <sys/wait.h>

// Constructor with shared map and mutex
APIServer::APIServer(unsigned long port, long timeout)
    : port(port), timeout(timeout),
      acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {}

// Start the API server
void APIServer::start() {
    std::cout << "Waiting for incoming connections on port " << port << std::endl;
    doAccept();
    io_context.run();
}

void APIServer::handleRequest(boost::asio::ip::tcp::socket socket) {
    auto socket_ptr = std::make_shared<boost::asio::ip::tcp::socket>(std::move(socket));
    auto buffer_ptr = std::make_shared<boost::asio::streambuf>();

    // Using shared pointer to keep `this` alive during the asynchronous operation
    auto self(shared_from_this());
    // Capture `data` by value and move it into the lambda
    boost::asio::async_read_until(*socket_ptr, *buffer_ptr, '\n',
        [this, self, buffer_ptr, socket_ptr](boost::system::error_code ec, std::size_t length) mutable {
            if (!ec) {
                std::istream stream(buffer_ptr.get());
                std::string command;
                std::getline(stream, command);
                // Process the command and get the response
                std::string response = processCommand(command);
                // Send the response back to the client
                boost::asio::async_write(*socket_ptr, boost::asio::buffer(response),
                    [socket_ptr](boost::system::error_code ec, std::size_t /*length*/) mutable {
                        if (!ec) {
                            // Close the socket after sending the response
                            socket_ptr->close();
                        } else {
                            std::cerr << "Error: " << ec.message() << std::endl;
                            socket_ptr->close();
                        }
                    });
            } else {
                std::cerr << "Error: " << ec.message() << std::endl;
                socket_ptr->close();
            }
        });
}

// Method to process individual commands
std::string APIServer::processCommand(const std::string& command) {
    std::stringstream ss(command);
    std::string action, uuid_str;
    ss >> action;

    if (action == "PING") {
        return "200 PONG\n";
    } else if (action == "ADD_SERVICE") {
        return addService(ss);
    } else if (action == "GET_INFO") {
        return getInfo(ss);
    }

    return "Error: Invalid command\n";
}

// Method to remove expired tokens
void APIServer::removeExpiredTokens() {
    boost::posix_time::ptime now = boost::posix_time::second_clock::universal_time();
    std::lock_guard<std::mutex> lock(tokens_mutex);
    for (auto it = tokens.begin(); it != tokens.end();) {
        if ((now - it->second.getTimestamp()).total_seconds() > timeout) {
            it = tokens.erase(it);
        } else {
            ++it;
        }
    }
}

// Internal method to accept incoming connections
void APIServer::doAccept() {
    acceptor.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                removeExpiredTokens(); // Remove expired tokens before processing new connection
                handleRequest(std::move(socket)); // Handle the new connection
            } else {
                std::cerr << "Error: " << ec.message() << std::endl;
            }
            doAccept(); // Accept the next connection
        });
}

// Method to add a new service and return the UUID
std::string APIServer::addService(std::stringstream& ss) {
    int port;
    ss >> port;

    if (ss.fail() || port <= 0 || port > 65535) {
        // Returning "400 Invalid Port" response
        return "400 Invalid Port\n";
    }
    // Generate a new UUID outside the lock
    UUID new_uuid;
    bool is_unique = false;
    // Try to generate a unique UUID
    while (!is_unique) {
        new_uuid = UUID(); // Generate a new UUID
        // Lock to check and add the UUID
        std::lock_guard<std::mutex> lock(tokens_mutex);
        if (tokens.find(new_uuid) == tokens.end()) {
            // Unique UUID found, add to tokens
            NetworkInfo network_info(port);
            tokens[new_uuid] = network_info;
            is_unique = true; // Exit loop
        }
    }
    // Returning "200" with the UUID as a response
    std::ostringstream response;
    response << "200 " << new_uuid.toString() << "\n";
    return response.str();
}

// Method to get information about a service using its UUID
std::string APIServer::getInfo(std::stringstream& ss) {
    std::string uuid_str;
    ss >> uuid_str;

    if (ss.fail()) {
        // Returning "400 Missing UUID" response
        return "400 Missing UUID\n";
    }
    // Convert the string to a UUID object
    UUID uuid;
    try {
        uuid = UUID(uuid_str);
    } catch (const std::invalid_argument& e) {
        // Returning "400 Invalid UUID" response
        return "400 Invalid UUID\n";
    }
    // Check if the UUID exists in the map
    std::lock_guard<std::mutex> lock(tokens_mutex);
    auto it = tokens.find(uuid);
    if (it == tokens.end()) {
        // Returning "404 UUID not found" response
        return "404 UUID not found\n";
    }
    // Retrieve the NetworkInfo associated with the UUID
    const NetworkInfo& network_info = it->second;
    int port = network_info.getPort();
    boost::posix_time::ptime timestamp = network_info.getTimestamp();
    // Calculate the time remaining before expiration
    boost::posix_time::ptime now = boost::posix_time::second_clock::universal_time();
    long seconds_elapsed = (now - timestamp).total_seconds();
    long time_remaining = timeout - seconds_elapsed;
    if (time_remaining <= 0) {
        // Returning "410 UUID has expired" response
        return "410 UUID has expired\n";
    }
    // Returning "200" with the port and time remaining as a response
    std::ostringstream response;
    response << "200 Port: " << port << " - Time remaining: " << time_remaining << " seconds\n";
    return response.str();
}