#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

// Helper function to trim leading and trailing whitespace
static char* trim(char* str) {
    if (str == NULL) return NULL;
    
    // Trim leading spaces
    while(isspace((unsigned char)*str)) str++;
    
    if(*str == 0) return str;  // All spaces
    
    // Trim trailing spaces
    char* end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    
    return str;
}

int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    if (cmd_line == NULL || clist == NULL) {
        return ERR_CMD_OR_ARGS_TOO_BIG;
    }

    // Initialize command list
    memset(clist, 0, sizeof(command_list_t));
    
    // Create a copy of cmd_line since strtok modifies the string
    char cmd_line_copy[ARG_MAX];
    strncpy(cmd_line_copy, cmd_line, ARG_MAX - 1);
    cmd_line_copy[ARG_MAX - 1] = '\0';
    
    // Trim the entire command line
    char *trimmed_cmd = trim(cmd_line_copy);
    
    // Check for empty command
    if (strlen(trimmed_cmd) == 0) {
        return WARN_NO_CMDS;
    }

    // First split by pipe
    char *saveptr1;
    char *pipe_token = strtok_r(trimmed_cmd, "|", &saveptr1);
    
    while (pipe_token != NULL) {
        // Check if we've exceeded maximum commands
        if (clist->num >= CMD_MAX) {
            return ERR_TOO_MANY_COMMANDS;
        }

        // Trim the command string
        char *cmd_str = trim(pipe_token);
        
        // Split command into executable and args
        char *saveptr2;
        char *token = strtok_r(cmd_str, " ", &saveptr2);
        
        if (token != NULL) {
            // Check executable length
            if (strlen(token) >= EXE_MAX) {
                return ERR_CMD_OR_ARGS_TOO_BIG;
            }
            strcpy(clist->commands[clist->num].exe, token);
            
            // Build args string
            char arg_buffer[ARG_MAX] = "";
            token = strtok_r(NULL, " ", &saveptr2);
            while (token != NULL) {
                if (strlen(arg_buffer) + strlen(token) + 2 >= ARG_MAX) {
                    return ERR_CMD_OR_ARGS_TOO_BIG;
                }
                if (strlen(arg_buffer) > 0) {
                    strcat(arg_buffer, " ");
                }
                strcat(arg_buffer, token);
                token = strtok_r(NULL, " ", &saveptr2);
            }
            
            if (strlen(arg_buffer) > 0) {
                strcpy(clist->commands[clist->num].args, arg_buffer);
            }
            
            clist->num++;
        }
        
        // Get next piped command
        pipe_token = strtok_r(NULL, "|", &saveptr1);
    }

    return OK;
}
