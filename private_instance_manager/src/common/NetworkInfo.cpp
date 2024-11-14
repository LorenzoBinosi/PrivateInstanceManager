#include "common/NetworkInfo.hpp"
#include <iostream>

// Default constructor - initializes port to 0 and time to current time (seconds resolution)
NetworkInfo::NetworkInfo()
    : endpoint_address_(""),
      endpoint_port_(0),
      timestamp_(boost::posix_time::second_clock::universal_time()) // Set current time in seconds
{}

// Constructor that accepts a port value and a service name. The service name is the name of the container. E.g.: docker_service:8080
NetworkInfo::NetworkInfo(std::string& service, long unsigned port)
    : endpoint_address_(service),
      endpoint_port_(port),
      timestamp_(boost::posix_time::second_clock::universal_time()) // Set current time in seconds
{}

// Getter for port
long unsigned NetworkInfo::getEndpointPort() const {
    return endpoint_port_;
}

// Getter for timestamp
boost::posix_time::ptime NetworkInfo::getTimestamp() const {
    return timestamp_;
}

// Getter for service
std::string NetworkInfo::getEndpointAddress() const {
    return endpoint_address_;
}

// Setter for port
void NetworkInfo::setEndpointPort(long unsigned port) {
    endpoint_port_ = port;
}

// Setter for timestamp
void NetworkInfo::setTimestamp(const boost::posix_time::ptime& timestamp) {
    timestamp_ = timestamp;
}

// Setter for service
void NetworkInfo::setEndpointAddress(std::string& endpoint_address) {
    endpoint_address_ = endpoint_address;
}