#include "API.hpp"
#include <boost/asio.hpp>
#include <iostream>
#include <sstream>

bool apiPing(unsigned long api_port) {
    int status;
    std::string answer;
    try {
        // Create an I/O context
        boost::asio::io_context io_context;
        // Create a socket
        boost::asio::ip::tcp::socket socket(io_context);
        // Resolve the host and port
        boost::asio::ip::tcp::resolver resolver(io_context);
        boost::asio::ip::tcp::resolver::results_type endpoints = resolver.resolve("127.0.0.1", std::to_string(api_port));
        // Connect to the server
        boost::asio::connect(socket, endpoints);
        // Send the message to the server
        std::string message = "PING\n";
        boost::asio::write(socket, boost::asio::buffer(message));
        // Read the response from the server until newline
        boost::asio::streambuf response_buffer;
        boost::asio::read_until(socket, response_buffer, "\n");
        std::istringstream response_stream(std::string(std::istreambuf_iterator<char>(&response_buffer), {}));
        response_stream >> status;
        if (!response_stream.fail() && status == 200) {
            response_stream >> answer;
            if (!response_stream.fail() && answer == "PONG") {
                socket.close();
                return true;
            }
        }
        socket.close();
    } catch (const std::exception& e) {
        std::cerr << "API request failed: " << e.what() << std::endl;
    }
    return false;
}


unsigned long apiGetInfoPort(unsigned long api_port, const std::string& uuid_str) {
    int status;
    std::string token;
    try {
        // Create an I/O context
        boost::asio::io_context io_context;
        // Create a socket
        boost::asio::ip::tcp::socket socket(io_context);
        // Resolve the host and port
        boost::asio::ip::tcp::resolver resolver(io_context);
        boost::asio::ip::tcp::resolver::results_type endpoints = resolver.resolve("127.0.0.1", std::to_string(api_port));
        // Connect to the server
        boost::asio::connect(socket, endpoints);
        // Send the message to the server
        std::string message = "GET_INFO " + uuid_str + "\n";
        boost::asio::write(socket, boost::asio::buffer(message));
        // Read the response from the server until newline
        boost::asio::streambuf response_buffer;
        boost::asio::read_until(socket, response_buffer, "\n");
        std::istringstream response_stream(std::string(std::istreambuf_iterator<char>(&response_buffer), {}));
        // Read the status code
        response_stream >> status;
        if (!response_stream.fail() && status == 200) {
            // Read the rest of the response
            std::string port_str;
            response_stream >> port_str;  // This should be "Port:"
            if (!response_stream.fail() && port_str == "Port:") {
                response_stream >> port_str;
                if (!response_stream.fail()) {
                    socket.close();
                    return std::stoul(port_str);
                }
            }
        }
        socket.close();
    } catch (const std::exception& e) {
        std::cerr << "API request failed: " << e.what() << std::endl;
    }
    return -1;
}


std::string apiAddService(unsigned long api_port, unsigned long service_port) {
    int status;
    std::string token;
    try {
        // Create an I/O context
        boost::asio::io_context io_context;
        // Create a socket
        boost::asio::ip::tcp::socket socket(io_context);
        // Resolve the host and port
        boost::asio::ip::tcp::resolver resolver(io_context);
        boost::asio::ip::tcp::resolver::results_type endpoints = resolver.resolve("127.0.0.1", std::to_string(api_port));
        // Connect to the server
        boost::asio::connect(socket, endpoints);
        // Send the message to the server
        std::string message = "ADD_SERVICE " + std::to_string(service_port) + "\n";
        boost::asio::write(socket, boost::asio::buffer(message));
        // Read the response from the server until newline
        boost::asio::streambuf response_buffer;
        boost::asio::read_until(socket, response_buffer, "\n");
        std::istringstream response_stream(std::string(std::istreambuf_iterator<char>(&response_buffer), {}));
        response_stream >> status;
        if (!response_stream.fail() && status == 200) {
            response_stream >> token;
        }
        // Close the socket
        socket.close();
    } catch (const std::exception& e) {
        std::cerr << "API request failed: " << e.what() << std::endl;
    }
    return token;
}