#include "common/NetworkInfo.hpp"
#include <iostream>

// Default constructor - initializes port to 0 and time to current time (seconds resolution)
NetworkInfo::NetworkInfo()
    : endpoint_(boost::asio::ip::tcp::v4(), 0), // Initialize endpoint with port 0
      timestamp_(boost::posix_time::second_clock::universal_time()) // Set current time in seconds
{}

// Constructor that accepts a port value and sets the timestamp to current time
NetworkInfo::NetworkInfo(int port)
    : endpoint_(boost::asio::ip::tcp::v4(), port), // Initialize endpoint with the given port
      timestamp_(boost::posix_time::second_clock::universal_time()) // Set current time in seconds
{}

// Getter for port
int NetworkInfo::getPort() const {
    return endpoint_.port();
}

// Getter for timestamp
boost::posix_time::ptime NetworkInfo::getTimestamp() const {
    return timestamp_;
}

// Setter for port
void NetworkInfo::setPort(int port) {
    endpoint_.port(port);
}

// Setter for timestamp
void NetworkInfo::setTimestamp(const boost::posix_time::ptime& timestamp) {
    timestamp_ = timestamp;
}