#include "utils/environ.hpp"
#include "clients/APIClient.hpp"
#include "servers/TunnelServer.hpp"
#include <boost/asio.hpp>
#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

// Forking server that tunnels the connection to a private instance
int main() {
    std::string instance_endpoint = get_string_env("INSTANCE_ENDPOINT", "localhost");
    unsigned long port = get_ulong_env("TUNNEL_PORT", 4002);
    std::string api_endpoint = get_string_env("API_ENDPOINT", "127.0.0.1");
    unsigned long api_port = get_ulong_env("API_PORT", 4001);
    boost::asio::io_context io_context;

    // Printing the configuration
    std::cout << "Starting tunnel server with the following configuration:" << std::endl;
    std::cout << "Instance endpoint: " << instance_endpoint << std::endl;
    std::cout << "Tunnel port: " << port << std::endl;
    std::cout << "API endpoint: " << api_endpoint << std::endl;
    std::cout << "API port: " << api_port << std::endl;

    // Determine the number of threads to use
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) {
        num_threads = 4; // Default to 4 threads if hardware_concurrency cannot determine
    }

    try {
        boost::asio::ip::tcp::acceptor acceptor(io_context,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
        std::function<void()> startAccept;
        // Start accepting connections asynchronously
        startAccept = [&](const boost::system::error_code& ec = {}) {
            if (!ec) {
                acceptor.async_accept(
                    [&](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                        if (!ec) {
                            // Instantiate TunnelServer for the new connection
                            std::make_shared<TunnelServer>(std::move(socket), api_endpoint, api_port, instance_endpoint)->start();
                        } else {
                            std::cerr << "Accept error: " << ec.message() << std::endl;
                        }
                        // Continue accepting new connections
                        startAccept();
                    });
            } else {
                std::cerr << "Start accept error: " << ec.message() << std::endl;
            }
        };
        startAccept();
        // Create a pool of threads to run the io_context
        std::vector<std::thread> threads;
        for (unsigned int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&io_context]() {
                io_context.run();
            });
        }
        // Optionally, run io_context in the main thread as well
        io_context.run();
        // Wait for all threads to finish
        for (auto& t : threads) {
            t.join();
        }
    } catch (std::exception& e) {
        std::cerr << "Exception in main: " << e.what() << std::endl;
    }
}
