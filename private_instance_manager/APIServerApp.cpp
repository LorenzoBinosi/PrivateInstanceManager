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

    // Start the API server
    auto apiServer = std::make_shared<APIServer>(api_port, timeout);
    apiServer->start();
    // In a container we can't use cin, so we just stop indefinitely
    while (true) {
        std::this_thread::sleep_for(std::chrono::hours(24 * 365));
    }
    return 0;
}
