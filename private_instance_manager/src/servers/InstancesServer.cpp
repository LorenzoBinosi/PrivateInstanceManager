#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <boost/process.hpp>
#include <boost/process/extend.hpp>
#include "servers/InstancesServer.hpp"
#include "clients/APIClient.hpp"
#include "utils/command_line.hpp"

unsigned short get_random_port() {
    int sockfd;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int opt = 1;
    unsigned short port;

    // Create socket (IPv4, TCP)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    // Optional: Set socket options
    // SO_REUSEADDR allows the socket to be bound to an address that is already in use
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(sockfd);
        return -1;
    }
    // Define the address
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; // Bind to all interfaces
    addr.sin_port = 0;                  // Let the OS assign a random free port
    // Bind the socket
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd);
        return -1;
    }
    // Retrieve the assigned port number
    if (getsockname(sockfd, (struct sockaddr *)&addr, &addr_len) == -1) {
        perror("getsockname");
        close(sockfd);
        return -1;
    }
    port = ntohs(addr.sin_port);
    close(sockfd);
    return port;
}

static APIClient& get_thread_api_client(const std::string& api_endpoint, unsigned short api_port) {
    thread_local std::unique_ptr<APIClient> api_client = nullptr;
    if (!api_client) {
        api_client = std::make_unique<APIClient>(api_endpoint, api_port);
    }
    return *api_client;
}


// Constructor to initialize the acceptor with the given port
InstancesServer::InstancesServer(unsigned short port, std::string& api_address, unsigned short api_port,
    long timeout, std::string& instance_address, std::string& command, std::string& challenge_address, 
    std::string& challenge_port, bool ssl, CommandType cmd_type, unsigned int user_id, unsigned int group_id, unsigned int num_threads)
    : port_(port),
      api_address_(api_address),
      api_port_(api_port),
      timeout_(timeout),
      acceptor_(io_context_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      instance_address_(instance_address),
      command_(command),
      challenge_address_(challenge_address),
      challenge_port_(challenge_port), 
      ssl_(ssl),
      user_id_(user_id),
      group_id_(group_id) {
    if (num_threads_ == 0) {
        num_threads_ = std::thread::hardware_concurrency();
        if (num_threads_ == 0) {
            num_threads_ = 1;
        }
    }
    if (cmd_type == CommandType::Docker) {
        run_command = [this](std::shared_ptr<boost::asio::ip::tcp::socket> client_socket) {
            this->runDockerCommand(client_socket);
        };
    } else {
        run_command = [this](std::shared_ptr<boost::asio::ip::tcp::socket> client_socket) {
            this->runBashCommand(client_socket);
        };
    }
}

void InstancesServer::runBashCommand(std::shared_ptr<boost::asio::ip::tcp::socket> client_socket) {
    boost::process::ipstream output;
    std::string line;
    uid_t user_id = user_id_;
    gid_t group_id = group_id_;

    auto self = shared_from_this();
    // Getting the API client
    APIClient& api_client = get_thread_api_client(api_address_, api_port_);
    // Dropping privileges
    auto on_setup_fn = [user_id, group_id]() {
        if (setgid(user_id) != 0) {
            perror("setgid failed");
            exit(1);
        }
        if (setuid(group_id) != 0) {
            perror("setuid failed");
            exit(1);
        }
    };
    // Running the command - Need to be a shared pointer
    auto process = std::make_shared<boost::process::child>(command_.c_str(), boost::process::std_out > output, boost::process::extend::on_setup(on_setup_fn));
    // Getting the port. Output is "Listening on port: <port>\n"
    std::getline(output, line);
    std::vector<std::string> tokens = split_command(line);
    if (tokens.size() < 4) {
        boost::asio::async_write(*client_socket, boost::asio::buffer("Failed to obtain a free port\n"),
            [client_socket](boost::system::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    std::cerr << "Write error: " << ec.message() << std::endl;
                }
                client_socket->close();
            });
        return;
    }
    unsigned short port = std::stoull(tokens[3]);
    // Getting the UUID
    auto result = api_client.apiAddService(instance_address_, port);
    if (!result) {
        boost::asio::async_write(*client_socket, boost::asio::buffer("Failed to add a service!\n"),
            [client_socket](boost::system::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    std::cerr << "Failed to write to client socket: " << ec.message() << std::endl;
                }
                client_socket->close();
            });
        return;
    }
    std::string token = *result;
    client_socket->write_some(boost::asio::buffer("Initialized private instance with token: " + token + "\n"));
    // Construct the challenge URL
    std::string connection_info = "ncat ";
    if (ssl_)
        connection_info += "--ssl ";
    connection_info += challenge_address_ + " " + challenge_port_;
    client_socket->write_some(boost::asio::buffer("Use it at: " + connection_info + "\n"));
    // Wait the timeout asynchronously
    auto timer = std::make_shared<boost::asio::steady_timer>(io_context_, std::chrono::seconds(timeout_));
    timer->async_wait([self, client_socket, process, timer] (const boost::system::error_code& ec) {
        if (!ec) {
            process->terminate();
            process->wait();
            boost::asio::async_write(*client_socket, boost::asio::buffer("Terminating the instance...\n"),
                [client_socket](boost::system::error_code ec, std::size_t /*length*/) {
                    if (ec) {
                        std::cerr << "Failed to write to client socket: " << ec.message() << std::endl;
                    }
                    client_socket->close();
                });
        }
        else
        {
            std::cerr << "Timer error: " << ec.message() << std::endl;
            client_socket->close();
        }
    });
}

void InstancesServer::runDockerCommand(std::shared_ptr<boost::asio::ip::tcp::socket> client_socket) {
    auto self = shared_from_this();
    // Getting the API client
    APIClient& api_client = get_thread_api_client(api_address_, api_port_);
    // Getting a free port
    unsigned short port = get_random_port();
    if (port == -1) {
        boost::asio::async_write(*client_socket, boost::asio::buffer("Failed to obtain a free port\n"),
            [client_socket](boost::system::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    std::cerr << "Write error: " << ec.message() << std::endl;
                }
                client_socket->close();
            });
    }
    // Getting the UUID
    auto result = api_client.apiAddService(instance_address_, port);
    if (!result) {
        boost::asio::async_write(*client_socket, boost::asio::buffer("Failed to add a service!\n"),
            [client_socket](boost::system::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    std::cerr << "Failed to write to client socket: " << ec.message() << std::endl;
                }
                client_socket->close();
            });
        return;
        
    }
    std::shared_ptr<std::string> token = std::make_shared<std::string>(*result);
    // Running the docker command
    char char_command[256] = {0}; // TODO - Dynamic allocation
    snprintf(char_command, sizeof(char_command), command_.c_str(), port, token->c_str());
    int status = system(char_command);
    if (status == -1) {
        boost::asio::async_write(*client_socket, boost::asio::buffer("Failed to run the docker command\n"),
            [client_socket](boost::system::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    std::cerr << "Failed to write to client socket: " << ec.message() << std::endl;
                }
                client_socket->close();
            });
        return;
    }
    client_socket->write_some(boost::asio::buffer("Initialized private instance with token: " + *token + "\n"));
    // Construct the challenge URL
    std::string connection_info = "ncat ";
    if (ssl_)
        connection_info += "--ssl ";
    connection_info += challenge_address_ + " " + challenge_port_;
    client_socket->write_some(boost::asio::buffer("Use it at: " + connection_info + "\n"));
    // Wait the timeout asynchronously
    auto timer = std::make_shared<boost::asio::steady_timer>(io_context_, std::chrono::seconds(timeout_));
    timer->async_wait([self, client_socket, token, timer](const boost::system::error_code& ec) {
        if (!ec) {
            // Stopping the docker container
            char char_command[256] = {0}; // TODO - Dynamic allocation
            snprintf(char_command, sizeof(char_command), "docker stop %s", token->c_str());
            int status = system(char_command);
            if (status == -1) {
                boost::asio::async_write(*client_socket, boost::asio::buffer("Failed to stop the instance\n"),
                    [client_socket](boost::system::error_code ec, std::size_t /*length*/) {
                        if (ec) {
                            std::cerr << "Failed to write to client socket: " << ec.message() << std::endl;
                        }
                        client_socket->close();
                    });
                return;
            }
            boost::asio::async_write(*client_socket, boost::asio::buffer("Terminating the instance...\n"),
                [client_socket](boost::system::error_code ec, std::size_t /*length*/) {
                    if (ec) {
                        std::cerr << "Failed to write to client socket: " << ec.message() << std::endl;
                    }
                    client_socket->close();
                });
        }
        else
        {
            std::cerr << "Timer error: " << ec.message() << std::endl;
            client_socket->close();
        }
    });

}

// Start method to run the server and handle incoming connections
void InstancesServer::start() {
    std::cout << "Server started." << std::endl;
    std::cout << "Using " << num_threads_ << " threads." << std::endl;
    std::cout << "Waiting for incoming connections on port " << port_ << "." << std::endl;
    doAccept();
    for (unsigned int i = 0; i < num_threads_; ++i) {
        threads_.emplace_back([this]() {
            io_context_.run();
        });
    }
}

void InstancesServer::stop() {
    io_context_.stop();
    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void InstancesServer::doAccept() {
    auto self = shared_from_this();
    acceptor_.async_accept(
        [this, self](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                // Handle the client connection
                handleClient(std::move(socket));
            } else {
                std::cerr << "Accept error: " << ec.message() << std::endl;
                socket.close();
            }
            // Continue accepting new connections
            doAccept();
        });
}

void InstancesServer::handleClient(boost::asio::ip::tcp::socket client_socket_) {
    // Creating a shared pointer for the client socket
    auto client_socket = std::make_shared<boost::asio::ip::tcp::socket>(std::move(client_socket_));
    client_socket->write_some(boost::asio::buffer("Initializing private instance...\n"));
    // Getting the API client
    run_command(client_socket);
}