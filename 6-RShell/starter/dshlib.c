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

