#include "utils/environ.hpp"
#include "clients/APIClient.hpp"
#include "servers/TunnelServer.hpp"
#include <boost/asio.hpp>
#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <thread>

// Forking server that tunnels the connection to a private instance
int main() {
    std::string instance_endpoint = get_string_env("INSTANCE_ADDRESS", "127.0.0.1");
    unsigned short port = get_ushort_env("TUNNEL_PORT", 4002);
    std::string api_endpoint = get_string_env("API_ADDRESS", "127.0.0.1");
    unsigned short api_port = get_ushort_env("API_PORT", 4001);

    // Start the tunnel server
    std::shared_ptr<TunnelServer> server = std::make_shared<TunnelServer>(port, api_endpoint, api_port);
    server->start();
    while (true) {
        std::this_thread::sleep_for(std::chrono::hours(24 * 365));
    }
    return 0;
}
