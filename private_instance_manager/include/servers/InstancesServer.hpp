#ifndef INSTANCESSERVER_HPP
#define INSTANCESSERVER_HPP

#include <boost/asio.hpp>
#include <string>
#include "clients/APIClient.hpp"

enum class CommandType {
    Docker,
    Bash
};

class InstancesServer : public std::enable_shared_from_this<InstancesServer> {
public:
    InstancesServer(unsigned short port, std::string& api_address, unsigned short api_port, long timeout, 
        std::string& instance_address, std::string& command, std::string& challenge_address, 
        std::string& challenge_port, bool ssl, CommandType cmd_type, unsigned int user_id, unsigned int group_id, unsigned int num_threads = 0);
    void start();
    void stop();

private:
    // Method to handle client connections
    void doAccept();
    void handleClient(boost::asio::ip::tcp::socket client_socket);
    void runDockerCommand(std::shared_ptr<boost::asio::ip::tcp::socket> client_socket);
    void runBashCommand(std::shared_ptr<boost::asio::ip::tcp::socket> client_socket);


    // Attributes
    unsigned short port_;
    std::string api_address_;
    unsigned short api_port_;
    long timeout_;
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::string instance_address_;
    std::string command_;
    std::string challenge_address_;
    std::string challenge_port_;
    bool ssl_;
    uid_t user_id_;
    gid_t group_id_;
    // Command to run
    std::function<void(std::shared_ptr<boost::asio::ip::tcp::socket>)> run_command;
    // Thread pool
    unsigned int num_threads_;
    std::vector<std::thread> threads_;
};

#endif // INSTANCESSERVER_HPP