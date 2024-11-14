#include "clients/APIClient.hpp"
#include <sstream>
#include <iostream>

APIClient::APIClient(const std::string& api_address, unsigned short api_port)
    : api_address_(api_address), api_port_(api_port), socket_(io_context_) {}

APIClient::~APIClient() {
    disconnect();
}

void APIClient::connect() {
    if (!socket_.is_open()) {
        boost::asio::ip::tcp::resolver resolver(io_context_);
        // std::cout << "Connecting to " << api_address_ << ":" << api_port_ << std::endl;
        auto endpoints = resolver.resolve(api_address_, std::to_string(api_port_));
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

std::optional<std::tuple<unsigned short, long, std::string>> APIClient::apiGetInfo(const std::string& uuid_str) {
    try {
        std::string response = sendRequest("GET_INFO " + uuid_str + "\n");
        if (response.empty()) {
            return std::make_tuple(0, 0, "");
        }
        std::istringstream response_stream(response);
        int status;
        std::string port_label, dash_label_1, dash_label_2, time_remaining_label, service_label, service_name;
        unsigned short port;
        long time_remaining;
        response_stream >> status >> port_label >> port \
            >> dash_label_1 >> time_remaining_label >> time_remaining \
            >> dash_label_2 >> service_label >> service_name;
        if (response_stream.fail() || status != 200 || port_label != "Port:" \
            || dash_label_1 != "-" || dash_label_2 != "-" || time_remaining_label != "TimeRemaining:"  \
            || service_label != "ServiceName:") {
            return std::nullopt;
        }
        // Everything is fine, return the NetworkInfo
        return std::make_tuple(port, time_remaining, service_name);
    } catch (const std::exception& e) {
        std::cerr << "Exception in apiGetInfoPort: " << e.what() << std::endl;
    }
    return std::nullopt;
}

std::optional<std::string> APIClient::apiAddService(const std::string& service_address, unsigned short service_port) {
    try {
        std::string response = sendRequest("ADD_SERVICE " + service_address + " " + std::to_string(service_port) + "\n");
        if (response.empty()) {
            return std::nullopt;
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
    return std::nullopt;
}