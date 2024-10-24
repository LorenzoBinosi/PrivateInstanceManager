#include "clients/APIClient.hpp"
#include <sstream>
#include <iostream>

APIClient::APIClient(const std::string& endpoint, unsigned long api_port)
    : endpoint_(endpoint), api_port_(api_port), socket_(io_context_) {}

APIClient::~APIClient() {
    disconnect();
}

void APIClient::connect() {
    if (!socket_.is_open()) {
        boost::asio::ip::tcp::resolver resolver(io_context_);
        std::cout << "Connecting to " << endpoint_ << ":" << api_port_ << std::endl;
        auto endpoints = resolver.resolve(endpoint_, std::to_string(api_port_));
        boost::asio::connect(socket_, endpoints);
    }
}

void APIClient::disconnect() {
    if (socket_.is_open()) {
        socket_.close();
    }
}

std::string APIClient::sendRequest(const std::string& message) {
    try {
        connect();
        boost::asio::write(socket_, boost::asio::buffer(message));

        boost::asio::streambuf response_buffer;
        boost::asio::read_until(socket_, response_buffer, "\n");
        disconnect();

        return std::string(
            std::istreambuf_iterator<char>(&response_buffer), {});
    } catch (const boost::system::system_error& e) {
        std::cerr << "Network error: " << e.what() << std::endl;
        return "";
    } catch (const std::exception& e) {
        std::cerr << "Error in sendRequest: " << e.what() << std::endl;
        return "";
    }
}

bool APIClient::apiPing() {
    try {
        std::string response = sendRequest("PING\n");
        if (response.empty()) {
            return false;
        }

        std::istringstream response_stream(response);
        int status;
        std::string answer;
        response_stream >> status >> answer;

        return !response_stream.fail() && status == 200 && answer == "PONG";
    } catch (const std::exception& e) {
        std::cerr << "Exception in apiPing: " << e.what() << std::endl;
        return false;
    }
}

unsigned long APIClient::apiGetInfoPort(const std::string& uuid_str) {
    try {
        std::string response = sendRequest("GET_INFO " + uuid_str + "\n");
        if (response.empty()) {
            return static_cast<unsigned long>(-1);
        }

        std::istringstream response_stream(response);
        int status;
        std::string port_label;
        unsigned long port;
        response_stream >> status >> port_label >> port;

        if (!response_stream.fail() && status == 200 && port_label == "Port:") {
            return port;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in apiGetInfoPort: " << e.what() << std::endl;
    }
    return static_cast<unsigned long>(-1);
}

std::string APIClient::apiAddService(unsigned long service_port) {
    try {
        std::string response = sendRequest("ADD_SERVICE " + std::to_string(service_port) + "\n");
        if (response.empty()) {
            return "";
        }

        std::istringstream response_stream(response);
        int status;
        std::string token;
        response_stream >> status >> token;

        if (!response_stream.fail() && status == 200) {
            return token;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in apiAddService: " << e.what() << std::endl;
    }
    return "";
}