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

