#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>  
#include "dshlib.h"
#include <errno.h>

extern void print_dragon(void);

/*
 * Allocates memory for a command buffer and initializes its fields.
 */
int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->argc = 0;
    cmd_buff->_cmd_buffer = NULL;
    
    // Initialize argument list
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    
    // Initialize redirection settings
    cmd_buff->in_redir_type = REDIR_NONE;
    cmd_buff->in_redir_file = NULL;
    cmd_buff->out_redir_type = REDIR_NONE;
    cmd_buff->out_redir_file = NULL;
    
    return OK;
}

/*
 * Frees allocated memory for a command buffer.
 */
int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    
    for (int i = 0; i < cmd_buff->argc; i++) {
        cmd_buff->argv[i] = NULL;
    }
    
    cmd_buff->argc = 0;
    return OK;
}

/*
 * Resets a command buffer for reuse without freeing memory.
 */
int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->argc = 0;
    if (cmd_buff->_cmd_buffer) {
        cmd_buff->_cmd_buffer[0] = '\0';
    }
    
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    
    return OK;
}

/*
 * Parses a command line into a command buffer, handling arguments and redirections.
 */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    size_t cmd_len = strlen(cmd_line);
    
    // Reset redirection settings
    cmd_buff->in_redir_type = REDIR_NONE;
    cmd_buff->in_redir_file = NULL;
    cmd_buff->out_redir_type = REDIR_NONE;
    cmd_buff->out_redir_file = NULL;
    
    // Allocate memory for command storage
    cmd_buff->_cmd_buffer = (char *)malloc(cmd_len + 1);
    if (!cmd_buff->_cmd_buffer) {
        return ERR_MEMORY;
    }
    
    // Copy command to buffer
    strcpy(cmd_buff->_cmd_buffer, cmd_line);
    
    // Detect redirection characters (<, >, >>) in the command
    char *in_redir = strchr(cmd_buff->_cmd_buffer, REDIR_IN_CHAR);
    char *out_redir = strchr(cmd_buff->_cmd_buffer, REDIR_OUT_CHAR);
    char *append_redir = strstr(cmd_buff->_cmd_buffer, ">>");

    // Handle output append redirection (>>)
    if (append_redir) {
        *append_redir = '\0';  // Terminate command at '>>'
        append_redir += 2;     // Move past '>>'
        
        // Skip leading whitespace in filename
        while (*append_redir && isspace(*append_redir)) {
            append_redir++;
        }
        
        if (*append_redir) {
            cmd_buff->out_redir_type = REDIR_APPEND;
            cmd_buff->out_redir_file = append_redir;
            
            // Remove trailing spaces from filename
            char *end = append_redir + strlen(append_redir) - 1;
            while (end > append_redir && isspace(*end)) {
                *end = '\0';
                end--;
            }
        } else {
            printf(CMD_ERR_REDIR);
            return ERR_CMD_ARGS_BAD;
        }
        
        out_redir = NULL; // Prevent duplicate handling
    }

    // Handle standard output redirection (>)
    if (out_redir && !append_redir) {
        *out_redir = '\0';  // Terminate command at '>'
        out_redir++;        // Move past '>'
        
        // Skip leading spaces in filename
        while (*out_redir && isspace(*out_redir)) {
            out_redir++;
        }
        
        if (*out_redir) {
            cmd_buff->out_redir_type = REDIR_OUT;
            cmd_buff->out_redir_file = out_redir;
            
            // Remove trailing spaces from filename
            char *end = out_redir + strlen(out_redir) - 1;
            while (end > out_redir && isspace(*end)) {
                *end = '\0';
                end--;
            }
        } else {
            printf(CMD_ERR_REDIR);
            return ERR_CMD_ARGS_BAD;
        }
    }

    // Handle input redirection (<)
    if (in_redir) {
        *in_redir = '\0';  // Terminate command at '<'
        in_redir++;        // Move past '<'
        
        // Skip leading spaces in filename
        while (*in_redir && isspace(*in_redir)) {
            in_redir++;
        }
        
        if (*in_redir) {
            cmd_buff->in_redir_type = REDIR_IN;
            cmd_buff->in_redir_file = in_redir;
            
            // Remove trailing spaces from filename
            char *end = in_redir + strlen(in_redir) - 1;
            while (end > in_redir && isspace(*end)) {
                *end = '\0';
                end--;
            }
        } else {
            printf(CMD_ERR_REDIR);
            return ERR_CMD_ARGS_BAD;
        }
    }

    // Tokenize the command line into arguments
    char *token = strtok(cmd_buff->_cmd_buffer, " \t");
    while (token != NULL && cmd_buff->argc < CMD_ARGV_MAX - 1) {
        cmd_buff->argv[cmd_buff->argc++] = token;
        token = strtok(NULL, " \t");
    }

    // Ensure null termination
    cmd_buff->argv[cmd_buff->argc] = NULL;
    
    return OK;
}

/*
 * Frees memory allocated for a command buffer.
 */
int close_cmd_buff(cmd_buff_t *cmd_buff) {
    return free_cmd_buff(cmd_buff);
}

/*
 * Identifies if a command is a built-in shell command.
 */
Built_In_Cmds match_command(const char *input) {
    if (!input || !*input) {
        return BI_NOT_BI;
    }
    
    if (strcmp(input, EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    }
    
    if (strcmp(input, "dragon") == 0) {
        return BI_CMD_DRAGON;
    }
    
    if (strcmp(input, "cd") == 0) {
        return BI_CMD_CD;
    }
    
    return BI_NOT_BI;
}

/*
 * Executes built-in commands directly in the shell process.
 */
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    if (!cmd || cmd->argc == 0) {
        return BI_NOT_BI;
    }
    
    Built_In_Cmds bi_cmd = match_command(cmd->argv[0]);
    
    switch (bi_cmd) {
        case BI_CMD_EXIT:
            printf("exiting...\n");
            return BI_CMD_EXIT;
        
        case BI_CMD_DRAGON:
            print_dragon();
            return BI_EXECUTED;
        
        case BI_CMD_CD:
            if (cmd->argc < 2) {
                char *home = getenv("HOME");
                if (home) {
                    chdir(home);
                }
            } else {
                if (chdir(cmd->argv[1]) != 0) {
                    perror("cd");
                }
            }
            return BI_EXECUTED;
        
        default:
            return BI_NOT_BI;
    }
}

/*
 * Build a list of commands from a command line, handling pipes
 */
int build_cmd_list(char *cmd_line, command_list_t *clist) {
    char *saveptr;
    char *token;
    char *cmd_str = strdup(cmd_line);
    int cmd_count = 0;
    
    if (!cmd_str) {
        return ERR_MEMORY;
    }
    
    // Initialize command list
    clist->num = 0;
    
    // Split command by pipe character
    token = strtok_r(cmd_str, PIPE_STRING, &saveptr);
    if (!token || strlen(token) == 0) {
        free(cmd_str);
        return WARN_NO_CMDS;
    }
    
    // Process each command in the pipeline
    while (token && cmd_count < CMD_MAX) {
        // Trim leading whitespace
        while (*token && isspace(*token)) {
            token++;
        }
        
        // Skip empty commands
        if (*token) {
            // Process this command
            int rc = build_cmd_buff(token, &clist->commands[cmd_count]);
            if (rc != OK) {
                free(cmd_str);
                return rc;
            }
            cmd_count++;
        }
        
        // Get next command
        token = strtok_r(NULL, PIPE_STRING, &saveptr);
    }
    
    // Check if we exceeded the maximum number of commands
    if (token) {
        free(cmd_str);
        return ERR_TOO_MANY_COMMANDS;
    }
    
    clist->num = cmd_count;
    free(cmd_str);
    
    return (cmd_count > 0) ? OK : WARN_NO_CMDS;
}

/*
 * Free the resources used by a command list
 */
int free_cmd_list(command_list_t *cmd_list) {
    if (!cmd_list) {
        return OK;
    }
    
    for (int i = 0; i < cmd_list->num; i++) {
        free_cmd_buff(&cmd_list->commands[i]);
    }
    
    cmd_list->num = 0;
    return OK;
}

/*
 * Execute a command pipeline locally
 */
int execute_pipeline(command_list_t *clist) {
    int n_cmds = clist->num;
    int pipes[CMD_MAX-1][2]; // Pipes for connecting commands
    pid_t pids[CMD_MAX];     // Process IDs for each command
    int status = 0;
    int rc = OK;
    
    // Create pipes
    for (int i = 0; i < n_cmds - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
    }
    
    // Execute commands
    for (int i = 0; i < n_cmds; i++) {
        pids[i] = fork();
        
        if (pids[i] < 0) {
            // Fork error
            perror("fork");
            rc = ERR_EXEC_CMD;
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
            }
            
            // Close all pipes
            for (int j = 0; j < n_cmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Execute the command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            
            // If execvp returns, it failed
            fprintf(stderr, "dsh: %s: command not found\n", clist->commands[i].argv[0]);
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
    
    return rc;
}

/*
 * Execute local commands (reusing from previous shell assignment)
 */
int exec_local_cmd_loop() {
    char cmd_line[SH_CMD_MAX];
    command_list_t cmd_list;
    int rc;
    
    while (1) {
        // Display prompt
        printf("%s", SH_PROMPT);
        fflush(stdout);
        
        // Read command
        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        
        // Remove newline
        cmd_line[strcspn(cmd_line, "\n")] = '\0';
        
        // Build command list
        memset(&cmd_list, 0, sizeof(command_list_t));
        rc = build_cmd_list(cmd_line, &cmd_list);
        
        if (rc == WARN_NO_CMDS) {
            printf(CMD_WARN_NO_CMD);
            continue;
        } else if (rc == ERR_TOO_MANY_COMMANDS) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            continue;
        } else if (rc != OK) {
            printf("Error parsing command: %d\n", rc);
            continue;
        }
        
        // Check for built-in commands (only check first command in pipeline)
        Built_In_Cmds bi_result = exec_built_in_cmd(&cmd_list.commands[0]);
        
        if (bi_result == BI_CMD_EXIT) {
            free_cmd_list(&cmd_list);
            return OK_EXIT;
        } else if (bi_result != BI_EXECUTED) {
            // Execute pipeline
            execute_pipeline(&cmd_list);
        }
        
        // Free resources
        free_cmd_list(&cmd_list);
    }
    
    return OK;
}
