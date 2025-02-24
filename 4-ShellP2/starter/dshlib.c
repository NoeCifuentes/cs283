#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>  // Added to fix missing `errno` issue
#include "dshlib.h"

/**
 * Trims leading and trailing spaces from a string.
 */
void trim_spaces(char *str) {
    char *start = str;
    while (*start == SPACE_CHAR) start++;  // Trim leading spaces

    // Move trimmed string back to the original buffer
    memmove(str, start, strlen(start) + 1);

    // Trim trailing spaces
    char *end = str + strlen(str) - 1;
    while (end > str && *end == SPACE_CHAR) {
        *end = '\0';
        end--;
    }
}

/**
 * Parses user input into `cmd_buff_t` structure.
 */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    trim_spaces(cmd_line);  // Remove leading/trailing spaces

    if (strlen(cmd_line) == 0) {
        return WARN_NO_CMDS;
    }

    // Allocate memory for the command buffer
    cmd_buff->_cmd_buffer = strdup(cmd_line);
    if (!cmd_buff->_cmd_buffer) {
        return ERR_MEMORY;
    }

    cmd_buff->argc = 0;
    char *token = strtok(cmd_buff->_cmd_buffer, " ");
    while (token && cmd_buff->argc < CMD_ARGV_MAX - 1) {
        cmd_buff->argv[cmd_buff->argc++] = token;
        token = strtok(NULL, " ");
    }
    cmd_buff->argv[cmd_buff->argc] = NULL;  // Null-terminate argument list

    return OK;
}

/**
 * Matches a command to a built-in type.
 */
Built_In_Cmds match_command(const char *input) {
    if (strcmp(input, EXIT_CMD) == 0) return BI_CMD_EXIT;
    if (strcmp(input, "cd") == 0) return BI_CMD_CD;
    if (strcmp(input, "rc") == 0) return BI_RC;  // Extra credit
    if (strcmp(input, "dragon") == 0) return BI_CMD_DRAGON;  // Extra credit
    return BI_NOT_BI;
}

/**
 * Executes built-in commands.
 */
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    if (cmd->argc == 0) return BI_NOT_BI;

    Built_In_Cmds cmd_type = match_command(cmd->argv[0]);

    switch (cmd_type) {
        case BI_CMD_EXIT:
            exit(0);

        case BI_CMD_CD:
            if (cmd->argc > 1) {
                if (chdir(cmd->argv[1]) != 0) {
                    perror("cd");
                }
            }
            return BI_EXECUTED;

        case BI_RC:
            printf("%d\n", errno);  // Extra credit: print last return code
            return BI_EXECUTED;

        case BI_CMD_DRAGON:
            print_dragon();  // Call the dragon function from dragon.c
            return BI_EXECUTED;

        default:
            return BI_NOT_BI;
    }
}

/**
 * Executes external commands using `fork()` and `execvp()`.
 */
int exec_cmd(cmd_buff_t *cmd) {
    if (cmd->argc == 0) {
        return WARN_NO_CMDS;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return ERR_EXEC_CMD;
    }

    if (pid == 0) {  // Child process
        execvp(cmd->argv[0], cmd->argv);
        perror("execvp");  // Only reached if execvp fails
        exit(errno);  // Pass error code to parent
    } else {  // Parent process
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);  // Extract return code from child process
        } else {
            return ERR_EXEC_CMD;
        }
    }
}

/**
 * Main shell loop to read, parse, and execute commands.
 */
int exec_local_cmd_loop() {
    char cmd_line[SH_CMD_MAX];
    cmd_buff_t cmd_buff;

    while (1) {
        printf("%s", SH_PROMPT);

        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }

        cmd_line[strcspn(cmd_line, "\n")] = '\0';  // Remove trailing newline

        int parse_result = build_cmd_buff(cmd_line, &cmd_buff);
        if (parse_result == WARN_NO_CMDS) {
            printf(CMD_WARN_NO_CMD);
            continue;
        }

        if (exec_built_in_cmd(&cmd_buff) == BI_NOT_BI) {
            int exec_result = exec_cmd(&cmd_buff);
            if (exec_result != 0) {
                printf("Command failed with exit code: %d\n", exec_result);
            }
        }

        free(cmd_buff._cmd_buffer);
    }

    return OK;
}

