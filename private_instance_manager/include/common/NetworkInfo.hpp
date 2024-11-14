#ifndef NETWORK_INFO_HPP
#define NETWORK_INFO_HPP

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

class NetworkInfo {
public:
    // Default constructor - sets port to 0 and timestamp to current time in seconds
    NetworkInfo();
    // Constructor with endpoint
    NetworkInfo(std::string& address, long unsigned port);
    // Getter for address
    std::string getEndpointAddress() const;
    // Getter for port
    long unsigned getEndpointPort() const;
    // Getter for timestamp
    boost::posix_time::ptime getTimestamp() const;
    // Setter for address
    void setEndpointAddress(std::string& address);
    // Setter for port
    void setEndpointPort(long unsigned port);
    // Setter for timestamp
    void setTimestamp(const boost::posix_time::ptime& timestamp);

private:
    std::string endpoint_address_;
    long unsigned endpoint_port_;
    boost::posix_time::ptime timestamp_;
};

#endif // NETWORK_INFO_HPP
