#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "RegisteringServer.hpp"
#include "API.hpp"

static void sigchld_handler(int /*signum*/) {
    while (waitpid(-1, nullptr, WNOHANG) > 0);
}

// Constructor to initialize the acceptor with the given port
RegisteringServer::RegisteringServer(unsigned long port, unsigned long api_port, long timeout)
    : port(port), api_port(api_port), timeout(timeout), acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {
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
void RegisteringServer::handleClient(boost::asio::ip::tcp::socket socket) {
    char timeout_arg[20]; // Ensure the buffer is large enough to hold the number
    snprintf(timeout_arg, sizeof(timeout_arg), "%ld", timeout);

    try {
        // Create a pipe for communication between parent and child
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            std::cerr << "Pipe creation failed!" << std::endl;
            return;
        }
        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "Fork failed!" << std::endl;
            close(pipefd[0]);
            close(pipefd[1]);
            return;
        } else if (pid == 0) {
            // Child process
            close(pipefd[0]);
            // Redirect stdout and stderr to the pipe
            if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                std::cerr << "dup2 failed!" << std::endl;
                exit(EXIT_FAILURE);
            }
            close(pipefd[1]);
            setvbuf(stdout, nullptr, _IONBF, 0);
            char* args[] = { const_cast<char*>("/usr/bin/timeout"),
                             timeout_arg,
                             const_cast<char*>("/usr/bin/tail"),
                             const_cast<char*>("-f"),
                             const_cast<char*>("/dev/null"),
                             nullptr };
            char* env[] = { nullptr };
            execve(args[0], args, env);
            std::cerr << "Execve failed!" << std::endl;
            exit(EXIT_FAILURE);
        } else {
            // Parent process
            // Buffer to store the child's output
            char c;
            std::string output;
            // Close the write end of the pipe
            close(pipefd[1]);
            socket.write_some(boost::asio::buffer("Initilizing private instance...\n"));
            // Read from the read end of the pipe
            // ssize_t bytes_read;
            // while ((bytes_read = read(pipefd[0], &c, 1)) > 0) {
            //     if (c == '\n') {
            //         break;
            //     }
            //     output += c;
            // }
            // Split the output to read the port number
            std::string token = apiAddService(api_port, 4003);
            socket.write_some(boost::asio::buffer("Initialized private instance with token: " + token + "\n"));
            // size_t pos = output.find("port: ");
            // if (pos != std::string::npos) {
            //     unsigned long instance_port = std::stoul(output.substr(pos + 6));
            //     // Send API request
            //     std::string token = apiAddService(instance_port, api_port);
            //     if (!token.empty()) {
            //         socket.write_some(boost::asio::buffer("Initialized private instance with token: " + token + "\n"));
            //         socket.write_some(boost::asio::buffer("Use it at: ncat --ssl https://blabla.com 8080\n"));
            //     } else {
            //         socket.write_some(boost::asio::buffer("Failed to initialize private instance\n"));
            //     }
            // } else {
            //     socket.write_some(boost::asio::buffer("Failed to initialize private instance\n"));
            // }
            close(pipefd[0]);
        }
    } catch (const std::exception& e) {
        std::cerr << "Client handling error: " << e.what() << std::endl;
    }
}