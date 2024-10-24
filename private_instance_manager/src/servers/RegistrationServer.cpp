#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "servers/RegistrationServer.hpp"
#include "clients/APIClient.hpp"
#include "utils/command_line.hpp"

static void sigchld_handler(int /*signum*/) {
    while (waitpid(-1, nullptr, WNOHANG) > 0);
}

// Constructor to initialize the acceptor with the given port
RegistrationServer::RegistrationServer(unsigned long port, std::string api_endpoint, unsigned long api_port,
    long timeout, std::string command, std::string challenge_endpoint, 
    std::string challenge_port, bool ssl)
    : port_(port), 
      api_port_(api_port), 
      timeout_(timeout),
      acceptor_(io_context_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      command_(command),
      challenge_endpoint_(challenge_endpoint),
      challenge_port_(challenge_port), 
      ssl_(ssl),
      client_(api_endpoint, api_port) {
    // Register signal handler for SIGCHLD
    std::signal(SIGCHLD, sigchld_handler);
}

// Start method to run the server and handle incoming connections
void RegistrationServer::start() {
    std::cout << "Waiting for incoming connections on port " << port_ << std::endl;
    while (true) {
        try {
            // Create a socket for the incoming connection
            boost::asio::ip::tcp::socket socket(io_context_);
            // Accept the next connection
            acceptor_.accept(socket);
            // Fork a new process to handle the client
            pid_t pid = fork();
            if (pid < 0) {
                std::cerr << "Fork failed!" << std::endl;
                socket.close();
            } else if (pid == 0) {
                // Child process
                io_context_.stop(); // Stop io_context in child
                acceptor_.close(); // Close the acceptor in child process
                handleClient(std::move(socket)); // Handle the client connection
                exit(0); // Exit the child process after handling the client
            } else {
                // Parent process
                socket.close(); // Close the client socket in the parent process
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
}

// Function to handle client connections
void RegistrationServer::handleClient(boost::asio::ip::tcp::socket client_socket) {
    try {
        // Create a pipe for communication between child and grandchild
        int port_pipe[2];
        int uuid_pipe[2];
        if (pipe(port_pipe) == -1) {
            perror("pipe failed");
            exit(EXIT_FAILURE);
        }
        if (pipe(uuid_pipe) == -1) {
            perror("pipe failed");
            close(port_pipe[0]);
            close(port_pipe[1]);
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid < 0) {
            // Fork failed
            perror("fork failed");
            close(port_pipe[0]);
            close(port_pipe[1]);
            close(uuid_pipe[0]);
            close(uuid_pipe[1]);
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Grandchild process
            close(port_pipe[0]); // Close read end of port_pipe
            close(client_socket.native_handle()); // Close the client socket

            // Obtain an available port
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                perror("socket failed");
                exit(EXIT_FAILURE);
            }

            struct sockaddr_in server_addr;
            socklen_t addr_len = sizeof(server_addr);
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = INADDR_ANY;
            server_addr.sin_port = 0; // Let OS choose port

            if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                perror("bind failed");
                close(sockfd);
                exit(EXIT_FAILURE);
            }

            if (getsockname(sockfd, (struct sockaddr *)&server_addr, &addr_len) < 0) {
                perror("getsockname failed");
                close(sockfd);
                exit(EXIT_FAILURE);
            }

            unsigned short port_number = ntohs(server_addr.sin_port);

            // Close the socket as it's no longer needed
            close(sockfd);

            // Write the port number to the port_pipe
            char port_str[16] = {0};
            snprintf(port_str, sizeof(port_str), "%u", port_number);
            if (write(port_pipe[1], port_str, strlen(port_str)) < 0) {
                perror("write to port_pipe failed");
                close(port_pipe[1]);
                close(uuid_pipe[0]);
                exit(EXIT_FAILURE);
            }
            close(port_pipe[1]); // Close write end after writing

            // Read the UUID from the uuid_pipe
            char uuid_str[64] = {0};
            ssize_t nbytes = read(uuid_pipe[0], uuid_str, sizeof(uuid_str) - 1);
            if (nbytes < 0) {
                std::cerr << "Failed to read UUID from uuid_pipe" << std::endl;
                close(uuid_pipe[0]);
                exit(EXIT_FAILURE);
            }
            close(uuid_pipe[0]); // Close read end after reading

            std::cout << "Starting private instance with port " << port_number << " and UUID " << uuid_str << std::endl;

            // Prepare the command to run
            // Substitute the port number in the command
            char char_command[0x100] = {0};
            snprintf(char_command, sizeof(char_command), command_.c_str(), port_number, uuid_str);
            std::string command_(char_command);
            // Split the Docker command
            std::vector<std::string> docker_args = split_command(command_);

            // Convert to char* array
            std::vector<char*> char_docker_args;
            for (auto& arg : docker_args) {
                char_docker_args.push_back(const_cast<char*>(arg.c_str()));
            }
            char_docker_args.push_back(nullptr); 

            for (auto& arg : char_docker_args) {
                std::cout << arg << " ";
            }
            std::cout << std::endl;

            // Execute the command
            if (execve("/usr/local/bin/docker", char_docker_args.data(), NULL) < 0) {
                perror("execve failed");
                exit(EXIT_FAILURE);
            }

            // Should not reach here
            exit(EXIT_FAILURE);
        } else {
            unsigned long service_port;

            // Child process (parent of grandchild)
            client_socket.write_some(boost::asio::buffer("Initilizing private instance...\n"));
            // close write end of port_pipe
            close(port_pipe[1]);

            // Read the port number from the grandchild
            char port_str[16];
            ssize_t nbytes = read(port_pipe[0], port_str, sizeof(port_str) - 1);
            if (nbytes > 0) {
                port_str[nbytes] = '\0';
                service_port = strtoul(port_str, NULL, 10);
            } else {
                client_socket.write_some(boost::asio::buffer("Initialization failed\n"));
                perror("Failed to read port number from grandchild");
                close(port_pipe[0]); 
                exit(EXIT_FAILURE);
            }
            close(port_pipe[0]); // Close read end of port_pipe


            // Registering the service
            std::string token = client_.apiAddService(service_port);
            if (!token.empty()) {
                client_socket.write_some(boost::asio::buffer("Initialized private instance with token: " + token + "\n"));
                // Construct the challenge URL
                std::string connection_info = "ncat ";
                if (ssl_)
                    connection_info += "--ssl ";
                connection_info += challenge_endpoint_ + " " + challenge_port_;
                client_socket.write_some(boost::asio::buffer("Use it at: " + connection_info + "\n"));
            } else {
                client_socket.write_some(boost::asio::buffer("Failed to initialize private instance\n"));
            }

            // Removing - from the token
            token.erase(std::remove(token.begin(), token.end(), '-'), token.end());
            // Writing the UUID to the uuid_pipe
            char uuid_str[64] = {0};
            snprintf(uuid_str, sizeof(uuid_str), "%s", token.c_str());
            std::cout << "UUID: " << uuid_str << std::endl;
            if (write(uuid_pipe[1], uuid_str, strlen(uuid_str)) < 0) {
                perror("write to uuid_pipe failed");
                close(uuid_pipe[1]);
                exit(EXIT_FAILURE);
            }
            close(uuid_pipe[1]); // Close write end after writing

            // Sleep for the timeout period
            sleep(timeout_);

            // Stop the docker container gracefully
            char* args[] = { const_cast<char*>("docker"), 
                     const_cast<char*>("stop"), 
                     const_cast<char*>(token.c_str()), 
                     nullptr };
            execve("/usr/local/bin/docker", args, nullptr);

            exit(0); // Child process exits
        }
    } catch (const std::exception& e) {
        std::cerr << "Client handling error: " << e.what() << std::endl;
    }
}
