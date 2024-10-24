#include <sstream>
#include "utils/command_line.hpp"

std::vector<std::string> split_command(const std::string& command) {
    std::vector<std::string> args;
    std::istringstream iss(command);
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }
    return args;
}