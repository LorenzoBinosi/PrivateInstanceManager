#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "RegisteringServer.hpp"
#include "API.hpp"

static void sigchld_handler(int /*signum*/) {
    while (waitpid(-1, nullptr, WNOHANG) > 0);
}

// Constructor to initialize the acceptor with the given port
RegisteringServer::RegisteringServer(unsigned long port, unsigned long api_port, long timeout, 
    std::string command, std::string challenge_endpoint, std::string challenge_port,
    bool ssl)
    : port(port), 
      api_port(api_port), 
      timeout(timeout),
      acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      command(command),
      challenge_endpoint(challenge_endpoint),
      challenge_port(challenge_port), 
      ssl(ssl) {
    // Register signal handler for SIGCHLD
    std::signal(SIGCHLD, sigchld_handler);
}

// Start method to run the server and handle incoming connections
void RegisteringServer::start() {
    std::cout << "Waiting for incoming connections on port " << port << std::endl;
    while (true) {
        try {
            // Create a socket for the incoming connection
            boost::asio::ip::tcp::socket socket(io_context);
            // Accept the next connection
            acceptor.accept(socket);
            // Fork a new process to handle the client
            pid_t pid = fork();
            if (pid < 0) {
                std::cerr << "Fork failed!" << std::endl;
                socket.close();
            } else if (pid == 0) {
                // Child process
                io_context.stop(); // Stop io_context in child
                acceptor.close(); // Close the acceptor in child process
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
void RegisteringServer::handleClient(boost::asio::ip::tcp::socket client_socket) {
    char timeout_arg[20]; // Ensure the buffer is large enough to hold the number
    snprintf(timeout_arg, sizeof(timeout_arg), "%ld", timeout);

    try {
        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "Fork failed!" << std::endl;
            return;
        } else if (pid == 0) {
            // Child process
            int sockfd;
            struct sockaddr_in server_addr;
            socklen_t addr_len = sizeof(server_addr);
            char port_str[6];
            char *bash_cmd[] = { const_cast<char*>("/bin/sh"), const_cast<char*>("-c"), NULL, NULL };

            // Create a TCP socket
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                perror("socket failed");
                exit(EXIT_FAILURE);
            }
            // Zero out the server_addr structure and set it up for IPv4
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = INADDR_ANY;
            server_addr.sin_port = 0;
            // Bind the socket
            if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                perror("bind failed");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            // Get the assigned port number
            if (getsockname(sockfd, (struct sockaddr *)&server_addr, &addr_len) < 0) {
                perror("getsockname failed");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            // Convert the port number to a string
            snprintf(port_str, sizeof(port_str), "%d", ntohs(server_addr.sin_port));
            // Close the socket as it's no longer needed
            close(sockfd);
            // Print the port number for debugging purposes
            printf("Assigned port: %s\n", port_str);
            // Prepare the socat command: "socat TCP-LISTEN:<port>,reuseaddr,fork EXEC:/bin/bash"
            char socat_command[0x100];
            snprintf(socat_command, sizeof(socat_command), command.c_str(), port_str);
            std::cout << "Executing command: " << socat_command << std::endl;
            bash_cmd[2] = socat_command; 
            // Execute "/bin/sh -c 'socat TCP-LISTEN:<port>,reuseaddr,fork EXEC:/bin/bash'"
            if (execve(const_cast<char*>("/bin/sh"), bash_cmd, NULL) < 0) {
                perror("execve failed");
                exit(EXIT_FAILURE);
            }
            exit(EXIT_FAILURE);
        } else {
            // Parent process
            client_socket.write_some(boost::asio::buffer("Initilizing private instance...\n"));
            // Split the output to read the port number
            std::string token = apiAddService(endpoint, api_port, 4003);
            if (!token.empty()) {
                client_socket.write_some(boost::asio::buffer("Initialized private instance with token: " + token + "\n"));
                // Construct the challenge URL
                std::string connection_info = "ncat ";
                if (ssl)
                    connection_info += "--ssl ";
                connection_info += challenge_endpoint + " " + challenge_port;
                client_socket.write_some(boost::asio::buffer("Use it at: " + connection_info + "\n"));
            } else {
                client_socket.write_some(boost::asio::buffer("Failed to initialize private instance\n"));
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Client handling error: " << e.what() << std::endl;
    }
}