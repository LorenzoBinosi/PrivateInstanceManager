#include "servers/InstancesServer.hpp"
#include "utils/environ.hpp"
#include "clients/APIClient.hpp"
#include <unordered_map>
#include <mutex>
#include <thread>
#include <iostream>

int main() {
    long timeout = get_long_env("TIMEOUT", 30);
    unsigned long server_port = get_ulong_env("SERVER_PORT", 4000);
    std::string api_endpoint = get_string_env("API_ENDPOINT", "127.0.0.1");
    unsigned long api_port = get_ulong_env("API_PORT", 4001);
    std::string command = get_string_env("COMMAND", "/bin/false");
    std::string challenge_endpoint = get_string_env("CHALLENGE_ENDPOINT", "127.0.0.1");
    std::string challenge_port = get_string_env("CHALLENGE_PORT", "8080");
    bool ssl = get_bool_env("SSL", false);

    // If first and last characters are quotes, remove them
    if ((command.front() == '"' && command.back() == '"') || (command.front() == '\'' && command.back() == '\'')) {
        command = command.substr(1, command.size() - 2);
    }

    // Start the API server
    InstancesServer server(server_port, api_endpoint, api_port, timeout, command, challenge_endpoint, challenge_port, ssl);

    std::cout << "Starting server with the following configuration:" << std::endl;
    std::cout << "Timeout: " << timeout << std::endl;
    std::cout << "Server port: " << server_port << std::endl;
    std::cout << "API port: " << api_port << std::endl;
    std::cout << "Command: " << command << std::endl;
    std::cout << "Challenge endpoint: " << challenge_endpoint << std::endl;
    std::cout << "Challenge port: " << challenge_port << std::endl;
    std::cout << "SSL: " << ssl << std::endl;
    

    // Wait for the API server to start
    APIClient client(api_endpoint, api_port);
    while (!client.apiPing()) {
        std::cout << "Waiting for API server to start on port " << api_port << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Start the registering server
    std::thread serverThread(&InstancesServer::start, &server);

    serverThread.join();

    return 0;
}
