#include "servers/InstancesServer.hpp"
#include "utils/environ.hpp"
#include "clients/APIClient.hpp"
#include "utils/strings.hpp"
#include <unordered_map>
#include <mutex>
#include <thread>
#include <iostream>

int main() {
    long timeout = get_long_env("TIMEOUT", 30);
    unsigned short server_port = get_ushort_env("SERVER_PORT", 4000);
    std::string api_address = get_string_env("API_ADDRESS", "127.0.0.1");
    unsigned short api_port = get_ushort_env("API_PORT", 4001);
    std::string docker_command = get_string_env("DOCKER_COMMAND", "");
    std::string bash_command = get_string_env("BASH_COMMAND", "");
    std::string challenge_address = get_string_env("CHALLENGE_ADDRESS", "127.0.0.1");
    std::string instances_address = get_string_env("INSTANCES_ADDRESS", "this_container"); // Name of the container that runs the instance
    std::string challenge_port = get_string_env("CHALLENGE_PORT", "8080");
    unsigned int user_id = get_uint_env("USER_UID", 1000);
    unsigned int group_id = get_uint_env("USER_GID", 1000);
    bool ssl = get_bool_env("SSL", false);

    // If none of the commands are provided, exit
    if (docker_command.empty() && bash_command.empty()) {
        std::cerr << "No command provided. Exiting..." << std::endl;
        return 1;
    }
    // Remove quotes from the commands
    docker_command = remove_quotes(docker_command);
    bash_command = remove_quotes(bash_command);
    // Determine the command
    std::string command;
    CommandType cmd_type;
    if (!docker_command.empty()) {
        cmd_type = CommandType::Docker;
        command = docker_command;
    } else if (!bash_command.empty()) {
        cmd_type = CommandType::Bash;
        command = bash_command;
    } else {
        std::cerr << "Invalid command provided. Exiting..." << std::endl;
        return 1;
    }
    // Start the API server
    std::shared_ptr<InstancesServer> server = std::make_shared<InstancesServer>(server_port, api_address, api_port, timeout, 
                           instances_address, command, challenge_address, challenge_port,
                           ssl, cmd_type, user_id, group_id);
    server->start();
    while (true) {
        std::this_thread::sleep_for(std::chrono::hours(24 * 365));
    }
    return 0;
}
