#ifndef NETWORK_INFO_HPP
#define NETWORK_INFO_HPP

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

class NetworkInfo {
public:
    // Default constructor - sets port to 0 and timestamp to current time in seconds
    NetworkInfo();
    // Constructor with endpoint
    NetworkInfo(int port);
    // Getter for port
    int getPort() const;
    // Getter for timestamp
    boost::posix_time::ptime getTimestamp() const;
    // Setter for port
    void setPort(int port);
    // Setter for timestamp
    void setTimestamp(const boost::posix_time::ptime& timestamp);

private:
    boost::asio::ip::tcp::endpoint endpoint_;
    boost::posix_time::ptime timestamp_;
};

#endif // NETWORK_INFO_HPP
