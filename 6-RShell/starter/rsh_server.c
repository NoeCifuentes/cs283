#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h> 

#include "dshlib.h"
#include "rshlib.h"

// Declaration for dragon function 
extern void print_dragon(void);

// Structure for thread arguments
typedef struct {
    int client_socket;
} thread_args_t;

// Flag to indicate if server should stop
volatile int server_should_stop = 0;
pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * start_server(ifaces, port, is_threaded)
 * Main server function - now supports multi-threading
 */
int start_server(char *ifaces, int port, int is_threaded) {
    int svr_socket;
    int rc = OK;
    
    // Initialize server mutex
    pthread_mutex_init(&server_mutex, NULL);
    server_should_stop = 0;

    // Boot up the server
    svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0) {
        int err_code = svr_socket;  
        return err_code;
    }

    // Process client requests - different handling based on threading mode
    if (is_threaded) {
        rc = process_threaded_requests(svr_socket);
    } else {
        rc = process_cli_requests(svr_socket);
    }

    // Stop the server
    stop_server(svr_socket);
    
    // Clean up mutex
    pthread_mutex_destroy(&server_mutex);

    return rc;
}

/*
 * stop_server(svr_socket)
 * Close server socket
 */
int stop_server(int svr_socket) {
    return close(svr_socket);
}

/*
 * boot_server(ifaces, port)
 * Initialize server socket
 */
int boot_server(char *ifaces, int port) {
    int svr_socket;
    int ret;
    struct sockaddr_in addr;
    
    // Create socket
    svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket < 0) {
        perror("socket");
        return ERR_RDSH_COMMUNICATION;
    }
    
    // Set socket options to reuse address
    int enable = 1;
    ret = setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (ret < 0) {
        perror("setsockopt");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Set up address info
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    // Convert IP string to network format
    if (inet_pton(AF_INET, ifaces, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Bind socket to address
    ret = bind(svr_socket, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        perror("bind");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Start listening for connections
    ret = listen(svr_socket, 20);
    if (ret == -1) {
        perror("listen");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    printf("Server listening on %s:%d\n", ifaces, port);
    return svr_socket;
}

/*
 * process_cli_requests(svr_socket)
 * Accept and process client connections
 */
int process_cli_requests(int svr_socket) {
    int cli_socket;
    int rc = OK;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    while(1) {
        // Accept client connection
        cli_socket = accept(svr_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (cli_socket < 0) {
            perror("accept");
            return ERR_RDSH_COMMUNICATION;
        }
        
        // Get client address information for logging
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("Client connected from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
        
        // Process client requests
        rc = exec_client_requests(cli_socket);
        
        // Close client socket
        close(cli_socket);
        
        // Check if we should stop the server
        if (rc == OK_EXIT) {
            printf("%s", RCMD_MSG_SVR_STOP_REQ);
            break;
        } else {
            printf("%s", RCMD_MSG_CLIENT_EXITED);
        }
    }

    return rc;
}

/*
 * process_threaded_requests(svr_socket)
 * Accept and process client connections in separate threads
 */
int process_threaded_requests(int svr_socket) {
    int cli_socket;
    pthread_t thread_id;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    thread_args_t *thread_args;
    
    // Set up server for non-blocking accept in case we need to check for shutdown
    int flags = fcntl(svr_socket, F_GETFL, 0);
    fcntl(svr_socket, F_SETFL, flags | O_NONBLOCK);

    while(!server_should_stop) {
        // Accept client connection (non-blocking)
        cli_socket = accept(svr_socket, (struct sockaddr*)&client_addr, &addr_len);
        
        if (cli_socket < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No connection available, sleep a bit and try again
                usleep(100000);  // 100ms
                continue;
            } else {
                perror("accept");
                return ERR_RDSH_COMMUNICATION;
            }
        }
        
        // Get client address information for logging
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("Client connected from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
        
        // Allocate thread arguments 
        thread_args = (thread_args_t*)malloc(sizeof(thread_args_t));
        if (!thread_args) {
            perror("malloc");
            close(cli_socket);
            continue;
        }
        thread_args->client_socket = cli_socket;
        
        // Create a new thread to handle this client
        if (pthread_create(&thread_id, NULL, client_thread, (void*)thread_args) != 0) {
            perror("pthread_create");
            free(thread_args);
            close(cli_socket);
            continue;
        }
        
        // Detach thread so it cleans up automatically when done
        pthread_detach(thread_id);
    }
    
    printf("Multi-threaded server stopping...\n");
    return OK_EXIT;
}

/*
 * client_thread(void *arg)
 * Thread function to handle a client connection
 */
void *client_thread(void *arg) {
    thread_args_t *thread_args = (thread_args_t*)arg;
    int cli_socket = thread_args->client_socket;
    int rc;
    
    // Process client requests
    rc = exec_client_requests(cli_socket);
    
    // Check if we should stop the server
    if (rc == OK_EXIT) {
        printf("%s", RCMD_MSG_SVR_STOP_REQ);
        pthread_mutex_lock(&server_mutex);
        server_should_stop = 1;
        pthread_mutex_unlock(&server_mutex);
    } else {
        printf("%s", RCMD_MSG_CLIENT_EXITED);
    }
    
    // Close client socket
    close(cli_socket);
    
    // Free thread arguments
    free(thread_args);
    
    return NULL;
}

/*
 * exec_client_requests(cli_socket)
 * Process commands from a connected client
 */
int exec_client_requests(int cli_socket) {
    char *recv_buff;
    command_list_t cmd_list;
    int retcode = OK;
    ssize_t byte_count;
    
    // Allocate receive buffer
    recv_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (!recv_buff) {
        perror("malloc");
        return ERR_RDSH_SERVER;
    }
    
    while (1) {
        // Reset buffer
        memset(recv_buff, 0, RDSH_COMM_BUFF_SZ);
        
        // Receive command from client
        byte_count = recv(cli_socket, recv_buff, RDSH_COMM_BUFF_SZ - 1, 0);
        if (byte_count <= 0) {
            // Client closed connection or error
            if (byte_count < 0) {
                perror("recv");
            }
            free(recv_buff);
            return OK;
        }
        
        // Ensure null termination
        recv_buff[byte_count] = '\0';
        
        // Handle empty commands
        if (strlen(recv_buff) == 0) {
            send_message_string(cli_socket, CMD_WARN_NO_CMD);
            send_message_eof(cli_socket);
            continue;
        }
        
        // Handle exit command
        if (strcmp(recv_buff, EXIT_CMD) == 0) {
            send_message_string(cli_socket, "Closing connection\n");
            send_message_eof(cli_socket);
            free(recv_buff);
            return OK;
        }
        
        // Handle stop-server command
        if (strcmp(recv_buff, "stop-server") == 0) {
            send_message_string(cli_socket, "Stopping server\n");
            send_message_eof(cli_socket);
            free(recv_buff);
            return OK_EXIT;
        }
        
        // Parse command list (handles pipes)
        memset(&cmd_list, 0, sizeof(command_list_t));
        retcode = build_cmd_list(recv_buff, &cmd_list);
        
        if (retcode == WARN_NO_CMDS) {
            send_message_string(cli_socket, CMD_WARN_NO_CMD);
            send_message_eof(cli_socket);
            continue;
        } else if (retcode == ERR_TOO_MANY_COMMANDS) {
            char buffer[100];
            sprintf(buffer, CMD_ERR_PIPE_LIMIT, CMD_MAX);
            send_message_string(cli_socket, buffer);
            send_message_eof(cli_socket);
            continue;
        } else if (retcode != OK) {
            send_message_string(cli_socket, CMD_ERR_RDSH_EXEC);
            send_message_eof(cli_socket);
            continue;
        }
        
        // Check for built-in commands (only process first command in pipeline)
        Built_In_Cmds builtin_result = BI_NOT_BI;
        
        if (cmd_list.num > 0) {
            builtin_result = match_command(cmd_list.commands[0].argv[0]);
            
            if (builtin_result == BI_CMD_EXIT) {
                send_message_string(cli_socket, "Closing connection\n");
                send_message_eof(cli_socket);
                free_cmd_list(&cmd_list);
                free(recv_buff);
                return OK;
            } else if (builtin_result == BI_CMD_DRAGON) {
                // Capture dragon ASCII art output
                FILE *temp_file = tmpfile();
                if (!temp_file) {
                    send_message_string(cli_socket, "Error creating temporary file for dragon output\n");
                    send_message_eof(cli_socket);
                    continue;
                }
                
                // Redirect stdout, print dragon, then get it back
                int stdout_fd = dup(STDOUT_FILENO);
                dup2(fileno(temp_file), STDOUT_FILENO);
                print_dragon();
                fflush(stdout);
                dup2(stdout_fd, STDOUT_FILENO);
                close(stdout_fd);
                
                // Send the dragon output to client
                rewind(temp_file);
                char dragon_buffer[RDSH_COMM_BUFF_SZ];
                size_t bytes_read;
                while ((bytes_read = fread(dragon_buffer, 1, RDSH_COMM_BUFF_SZ - 1, temp_file)) > 0) {
                    dragon_buffer[bytes_read] = '\0';
                    send_message_string(cli_socket, dragon_buffer);
                }
                
                fclose(temp_file);
                send_message_eof(cli_socket);
                
                builtin_result = BI_EXECUTED;
            } else if (builtin_result == BI_CMD_CD) {
                // Handle cd command
                if (cmd_list.commands[0].argc < 2) {
                    char *home = getenv("HOME");
                    if (home && chdir(home) == 0) {
                        send_message_string(cli_socket, "Changed directory to HOME\n");
                    } else {
                        send_message_string(cli_socket, "Failed to change to HOME directory\n");
                    }
                } else {
                    if (chdir(cmd_list.commands[0].argv[1]) == 0) {
                        char buffer[512];
                        sprintf(buffer, "Changed directory to %s\n", cmd_list.commands[0].argv[1]);
                        send_message_string(cli_socket, buffer);
                    } else {
                        char buffer[512];
                        sprintf(buffer, "Failed to change to directory %s: %s\n", 
                                cmd_list.commands[0].argv[1], strerror(errno));
                        send_message_string(cli_socket, buffer);
                    }
                }
                send_message_eof(cli_socket);
                builtin_result = BI_EXECUTED;
            }
        }
        
        // If not a built-in command, execute the pipeline
        if (builtin_result != BI_EXECUTED) {
            retcode = rsh_execute_pipeline(cli_socket, &cmd_list);
            if (retcode != OK) {
                send_message_string(cli_socket, CMD_ERR_RDSH_EXEC);
                send_message_eof(cli_socket);
            }
        }
        
        // Free command list resources
        free_cmd_list(&cmd_list);
    }
    
    free(recv_buff);
    return OK;
}

/*
 * send_message_string(cli_socket, buff)
 * Send a string to the client
 */
int send_message_string(int cli_socket, char *buff) {
    ssize_t bytes_sent = 0;
    size_t message_len = strlen(buff);
    
    bytes_sent = send(cli_socket, buff, message_len, 0);
    if (bytes_sent < 0) {
        perror("send");
        return ERR_RDSH_COMMUNICATION;
    } else if ((size_t)bytes_sent != message_len) {
        fprintf(stderr, CMD_ERR_RDSH_SEND, (int)bytes_sent, (int)message_len);
        return ERR_RDSH_COMMUNICATION;
    }
    
    return OK;
}

/*
 * send_message_eof(cli_socket)
 * Send EOF character to signal end of message
 */
int send_message_eof(int cli_socket) {
    ssize_t bytes_sent = send(cli_socket, &RDSH_EOF_CHAR, 1, 0);
    if (bytes_sent != 1) {
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}

/*
 * rsh_execute_pipeline(socket_fd, clist)
 * Execute command pipeline with output redirected to socket
 */
int rsh_execute_pipeline(int socket_fd, command_list_t *clist) {
    int n_cmds = clist->num;
    int pipes[CMD_MAX-1][2]; // Pipes for connecting commands
    pid_t pids[CMD_MAX];     // Process IDs for each command
    int status = 0;
    int rc = OK;
    
    // Create pipes
    for (int i = 0; i < n_cmds - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_RDSH_CMD_EXEC;
        }
    }
    
    // Execute commands
    for (int i = 0; i < n_cmds; i++) {
        pids[i] = fork();
        
        if (pids[i] < 0) {
            // Fork error
            perror("fork");
            rc = ERR_RDSH_CMD_EXEC;
            break;
            
        } else if (pids[i] == 0) {
            // Child process
            
            // Handle stdin (either from previous pipe or original stdin)
            if (i > 0) {
                // Read from previous pipe
                dup2(pipes[i-1][0], STDIN_FILENO);
            } else if (clist->commands[i].in_redir_type == REDIR_IN) {
                // Input redirection
                int fd = open(clist->commands[i].in_redir_file, O_RDONLY);
                if (fd < 0) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            
            // Handle stdout (either to next pipe or original stdout)
            if (i < n_cmds - 1) {
                // Write to next pipe
                dup2(pipes[i][1], STDOUT_FILENO);
            } else if (clist->commands[i].out_redir_type == REDIR_OUT) {
                // Output redirection
                int fd = open(clist->commands[i].out_redir_file, 
                              O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            } else if (clist->commands[i].out_redir_type == REDIR_APPEND) {
                // Append redirection
                int fd = open(clist->commands[i].out_redir_file, 
                              O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (fd < 0) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            } else {
                // For the last command in the pipeline, redirect stdout to socket
                if (i == n_cmds - 1) {
                    dup2(socket_fd, STDOUT_FILENO);
                }
            }
            
            // Always redirect stderr to socket
            dup2(socket_fd, STDERR_FILENO);
            
            // Close all pipes
            for (int j = 0; j < n_cmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Execute the command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            
            // If execvp returns, it failed
            fprintf(stderr, "rdsh: %s: command not found\n", clist->commands[i].argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    
    // Parent process
    // Close all pipe file descriptors
    for (int i = 0; i < n_cmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all child processes to complete
    for (int i = 0; i < n_cmds; i++) {
        waitpid(pids[i], &status, 0);
    }
    
    // Send EOF to indicate end of output
    send_message_eof(socket_fd);
    
    return rc;
}
