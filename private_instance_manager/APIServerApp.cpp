#include "servers/APIServer.hpp"
#include "common/UUID.hpp"
#include "clients/APIClient.hpp"
#include "utils/environ.hpp"
#include <unordered_map>
#include <mutex>
#include <thread>

int main() {
    long timeout = get_long_env("TIMEOUT", 30);
    unsigned long api_port = get_ulong_env("API_PORT", 4001);

    // Printing the configuration
    std::cout << "Starting server with the following configuration:" << std::endl;
    std::cout << "Timeout: " << timeout << std::endl;
    std::cout << "API port: " << api_port << std::endl;

    // Start the API server
    auto apiServer = std::make_shared<APIServer>(api_port, timeout);
    std::thread apiServerThread(&APIServer::start, apiServer);

    // Wait for the API server to finish
    apiServerThread.join();

    return 0;
}
