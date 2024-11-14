// APIServer.cpp

#include "servers/APIServer.hpp"
#include <iostream>
#include <sstream>

APIServer::APIServer(unsigned short port, long timeout_seconds, long cleanup_interval, unsigned short num_threads)
    : port_(port),
      timeout_seconds_(timeout_seconds),
      cleanup_interval_(cleanup_interval),
      acceptor_(io_context_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      num_threads_(num_threads) {
    if (num_threads_ == 0) {
        num_threads_ = std::thread::hardware_concurrency();
        if (num_threads_ == 0) {
            num_threads_ = 1;
        }
    }
}


void APIServer::start() {
    std::cout << "Server started." << std::endl;
    scheduleTokenCleanup();
    std::cout << "Token cleanup scheduled every " << cleanup_interval_ << " seconds." << std::endl;
    std::cout << "Using " << num_threads_ << " threads." << std::endl;
    std::cout << "Waiting for incoming connections on port " << port_ << "." << std::endl;
    doAccept();
    for (unsigned int i = 0; i < num_threads_; ++i) {
        threads_.emplace_back([this]() {
            io_context_.run();
        });
    }
}

void APIServer::stop() {
    io_context_.stop();
    // Wait for all threads to finish
    for (auto& t : threads_) {
        t.join();
    }
}

void APIServer::scheduleTokenCleanup() {
    auto self = shared_from_this();
    cleanup_timer_ = std::make_shared<boost::asio::deadline_timer>(io_context_, boost::posix_time::seconds(cleanup_interval_));
    cleanup_timer_->async_wait([self](const boost::system::error_code& ec) {
        if (!ec) {
            self->removeExpiredTokens();
            self->scheduleTokenCleanup();
        }
    });
}

void APIServer::doAccept() {
    auto self = shared_from_this();
    acceptor_.async_accept(
        [this, self](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                try {
                    handleRequest(std::move(socket));
                } catch (const std::exception& e) {
                    std::cerr << "Exception in connection handler: " << e.what() << std::endl;
                }
            } else {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }
            doAccept(); // Accept the next connection
        });
}

void APIServer::handleRequest(boost::asio::ip::tcp::socket socket) {
    auto self = shared_from_this();
    auto socket_ptr = std::make_shared<boost::asio::ip::tcp::socket>(std::move(socket));
    auto buffer_ptr = std::make_shared<boost::asio::streambuf>();

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
                        if (ec) {
                            std::cerr << "Error: " << ec.message() << std::endl;
                            
                        }
                        socket_ptr->close();
                    });
            } else {
                std::cerr << "Error: " << ec.message() << std::endl;
                socket_ptr->close();
            }
        });
}

std::string APIServer::processCommand(const std::string& command) {
    std::stringstream ss(command);
    std::string action;
    ss >> action;

    if (ss.fail()) {
        return statusMessage(StatusCode::BadRequest) + " Invalid command\n";
    }

    if (action == "PING") {
        return statusMessage(StatusCode::OK) + " PONG\n";
    } else if (action == "ADD_SERVICE") {
        return addService(ss);
    } else if (action == "GET_INFO") {
        return getInfo(ss);
    }

    return statusMessage(StatusCode::BadRequest) + " Invalid command\n";
}

std::string APIServer::addService(std::stringstream& ss) {
    unsigned short port;
    std::string address;

    ss >> address;
    if (ss.fail()) {
        return statusMessage(StatusCode::BadRequest) + " Missing service name\n";
    }
    ss >> port;
    if (ss.fail() || port == 0) {
        return statusMessage(StatusCode::BadRequest) + " Invalid port number\n";
    }
    // Generate a new UUID outside the lock
    UUID new_uuid;
    bool is_unique = false;
    // Try to generate a unique UUID
    while (!is_unique) {
        new_uuid = UUID(); // Generate a new UUID
        // Lock to check and add the UUID
        std::lock_guard<std::mutex> lock(tokens_mutex_);
        if (tokens_.find(new_uuid) == tokens_.end()) {
            // Unique UUID found, add to tokens
            NetworkInfo network_info(address, port);
            tokens_[new_uuid] = network_info;
            is_unique = true; // Exit loop
        }
    }

    // Returning "200" with the UUID as a response
    std::ostringstream response;
    response << "200 " << new_uuid.toString() << "\n";
    return response.str();
}

std::string APIServer::getInfo(std::stringstream& ss) {
    std::string uuid_str;
    ss >> uuid_str;

    if (ss.fail()) {
        return statusMessage(StatusCode::BadRequest) + " Missing UUID\n";
    }

    UUID uuid;
    try {
        uuid = UUID(uuid_str);
    } catch (const std::invalid_argument& e) {
        return statusMessage(StatusCode::BadRequest) + " Invalid UUID\n";
    }

    // Retrieve the NetworkInfo associated with the UUID
    NetworkInfo network_info;
    {
        std::lock_guard<std::mutex> lock(tokens_mutex_);
        auto it = tokens_.find(uuid);
        if (it == tokens_.end()) {
            return statusMessage(StatusCode::NotFound) + " UUID not found\n";
        }
        network_info = it->second;
    }

    // Calculate the time remaining before expiration
    boost::posix_time::ptime now = boost::posix_time::second_clock::universal_time();
    long seconds_elapsed = (now - network_info.getTimestamp()).total_seconds();
    long time_remaining = timeout_seconds_ - seconds_elapsed;
    if (time_remaining <= 0) {
        return statusMessage(StatusCode::Gone) + " UUID has expired\n";
    }
    // Returning "200 Port: <port> - TimeRemaining: <seconds> - ServiceName: <service_name>\n"
    std::ostringstream response;
    response << statusMessage(StatusCode::OK)
             << " Port: " << network_info.getEndpointPort()
             << " - TimeRemaining: " << time_remaining 
             << " - ServiceName: " << network_info.getEndpointAddress() << "\n";
    return response.str();
}

void APIServer::removeExpiredTokens() {
    boost::posix_time::ptime now = boost::posix_time::second_clock::universal_time();
    std::lock_guard<std::mutex> lock(tokens_mutex_);
    for (auto it = tokens_.begin(); it != tokens_.end();) {
        long seconds_elapsed = (now - it->second.getTimestamp()).total_seconds();
        if (seconds_elapsed > timeout_seconds_) {
            it = tokens_.erase(it);
        } else {
            ++it;
        }
    }
}

std::string APIServer::statusMessage(StatusCode code) {
    switch (code) {
        case StatusCode::OK:
            return "200";
        case StatusCode::BadRequest:
            return "400";
        case StatusCode::NotFound:
            return "404";
        case StatusCode::Gone:
            return "410";
        case StatusCode::InternalServerError:
            return "500";
        default:
            return "500";
    }
}
