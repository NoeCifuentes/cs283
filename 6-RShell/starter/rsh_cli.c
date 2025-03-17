#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>

#include "dshlib.h"
#include "rshlib.h"

/*
 * exec_remote_cmd_loop(server_ip, port)
 * 
 * Implements the client-side functionality for the Remote Drexel Shell.
 * - Establishes a TCP connection to the remote shell server.
 * - Reads user commands from stdin and sends them to the server.
 * - Receives and prints server responses.
 * - Detects and handles client exit conditions.
 */
int exec_remote_cmd_loop(char *address, int port)
{
    char *cmd_buff;  // Buffer to hold user commands
    char *rsp_buff;  // Buffer to store server responses
    int cli_socket;  // Client socket file descriptor
    ssize_t io_size; // Variable to track bytes sent/received
    int is_eof;      // Flag to check if EOF marker is received

    // Allocate memory for command buffer
    cmd_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (cmd_buff == NULL) {
        return client_cleanup(-1, NULL, NULL, ERR_MEMORY);
    }

    // Allocate memory for response buffer
    rsp_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (rsp_buff == NULL) {
        return client_cleanup(-1, cmd_buff, NULL, ERR_MEMORY);
    }

    // Create client socket and connect to server
    cli_socket = start_client(address, port);
    if (cli_socket < 0) {
        perror("start client");
        return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_CLIENT);
    }

    while (1) 
    {
        // Display shell prompt
        printf("%s", SH_PROMPT);
        fflush(stdout);

        // Read user input
        if (fgets(cmd_buff, RDSH_COMM_BUFF_SZ, stdin) == NULL) {
            printf("\n"); // Handle Ctrl+D gracefully
            break;
        }

        // Remove newline character from user input
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        // Ignore empty input
        if (strlen(cmd_buff) == 0) {
            continue;
        }

        // Handle exit command
        if (strcmp(cmd_buff, EXIT_CMD) == 0) {
            // Notify the server before exiting
            int send_len = strlen(cmd_buff) + 1; // Include null terminator
            io_size = send(cli_socket, cmd_buff, send_len, 0);
            if (io_size != send_len) {
                perror("send");
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
            }
            break; // Exit client loop
        }

        // Send command to the server
        int send_len = strlen(cmd_buff) + 1; // Include null terminator
        io_size = send(cli_socket, cmd_buff, send_len, 0);
        if (io_size != send_len) {
            perror("send");
            return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
        }

        // Receive and display server responses
        do {
            io_size = recv(cli_socket, rsp_buff, RDSH_COMM_BUFF_SZ - 1, 0);
            
            if (io_size < 0) {
                perror("recv");
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
            } else if (io_size == 0) {
                // Server closed the connection
                printf("%s", RCMD_SERVER_EXITED);
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
            }

            // Check if received data contains EOF marker
            is_eof = (rsp_buff[io_size - 1] == RDSH_EOF_CHAR) ? 1 : 0;
            
            if (is_eof) {
                // Print output excluding EOF character
                printf("%.*s", (int)io_size - 1, rsp_buff);
            } else {
                // Print entire response buffer
                printf("%.*s", (int)io_size, rsp_buff);
            }
            
            // Flush output to ensure immediate display
            fflush(stdout);
            
        } while (!is_eof);
    }

    return client_cleanup(cli_socket, cmd_buff, rsp_buff, OK);
}

/*
 * start_client(server_ip, port)
 * 
 * Establishes a TCP connection to the remote shell server.
 * - Creates a socket using `socket()`.
 * - Uses `inet_pton()` to convert IP address from text to binary form.
 * - Calls `connect()` to establish a connection with the server.
 * 
 * Returns:
 * - On success: A valid socket descriptor.
 * - On failure: An error code.
 */
int start_client(char *server_ip, int port) {
    struct sockaddr_in addr;
    int cli_socket;
    int ret;

    // Create a TCP socket
    cli_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (cli_socket < 0) {
        perror("socket");
        return ERR_RDSH_CLIENT;
    }

    // Configure server address structure
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(cli_socket);
        return ERR_RDSH_CLIENT;
    }

    // Connect to the server
    ret = connect(cli_socket, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        perror("connect");
        close(cli_socket);
        return ERR_RDSH_CLIENT;
    }

    return cli_socket;
}

/*
 * client_cleanup(cli_socket, cmd_buff, rsp_buff, rc)
 * 
 * Cleans up client resources before terminating.
 * - Closes the client socket if it is open.
 * - Frees allocated memory for command and response buffers.
 * - Returns the provided status code.
 */
int client_cleanup(int cli_socket, char *cmd_buff, char *rsp_buff, int rc) {
    // Close the socket if it is open
    if (cli_socket > 0) {
        close(cli_socket);
    }

    // Free allocated command buffer
    if (cmd_buff != NULL) {
        free(cmd_buff);
    }
    
    // Free allocated response buffer
    if (rsp_buff != NULL) {
        free(rsp_buff);
    }

    // Return the given status code
    return rc;
}

