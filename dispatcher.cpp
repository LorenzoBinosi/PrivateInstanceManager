#include "RegisteringServer.hpp"
#include "APIServer.hpp"
#include "UUID.hpp"
#include "API.hpp"
#include "Utils.hpp"
#include <unordered_map>
#include <mutex>
#include <thread>

int main() {
    long timeout = get_long_env("TIMEOUT", 30);
    unsigned long server_port = get_ulong_env("SERVER_PORT", 4000);
    unsigned long api_port = get_ulong_env("API_PORT", 4001);

    auto apiServer = std::make_shared<APIServer>(api_port, timeout);
    RegisteringServer server(server_port, api_port, timeout);
    
    std::thread apiServerThread(&APIServer::start, apiServer);
    while (!apiPing(api_port)) {
        std::cout << "Waiting for API server to start on port " << api_port << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::thread serverThread(&RegisteringServer::start, &server);

    apiServerThread.join();
    serverThread.join();

    return 0;
}
