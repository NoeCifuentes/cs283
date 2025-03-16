#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include "dshlib.h"

extern void print_dragon(void);

/*
 * Main shell loop
 */
int exec_local_cmd_loop() {
    char cmd_buff[SH_CMD_MAX];
    command_list_t clist;

    while (1) {
        printf("%s", SH_PROMPT);
        fflush(stdout);

        if (!fgets(cmd_buff, SH_CMD_MAX, stdin)) {
            break; // Exit on EOF
        }

        cmd_buff[strcspn(cmd_buff, "\n")] = '\0'; // Remove trailing newline

        if (strlen(cmd_buff) == 0) {
            printf("%s", CMD_WARN_NO_CMD);
            continue;
        }

        if (strcmp(cmd_buff, EXIT_CMD) == 0) {
            printf("exiting...\n");
            break;
        }

        int rc = build_cmd_list(cmd_buff, &clist);
        if (rc != OK) {
            continue;
        }

        rc = execute_pipeline(&clist);
        free_cmd_list(&clist);

        if (rc == OK_EXIT) {
            break;
        }
    }
    return 0;
}

/*
 * Execute a single command (with redirection)
 */
int exec_cmd(cmd_buff_t *cmd) {
    if (cmd->argc == 0) {
        return ERR_CMD_ARGS_BAD;
    }

    Built_In_Cmds bi_result = exec_built_in_cmd(cmd);
    if (bi_result == BI_CMD_EXIT) {
        return OK_EXIT;
    } else if (bi_result == BI_EXECUTED) {
        return OK;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return ERR_EXEC_CMD;
    } else if (pid == 0) { // Child process
        if (cmd->in_redir_type == REDIR_IN) {
            int fd = open(cmd->in_redir_file, O_RDONLY);
            if (fd < 0) {
                perror("open input");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        if (cmd->out_redir_type != REDIR_NONE) {
            int flags = O_WRONLY | O_CREAT | ((cmd->out_redir_type == REDIR_APPEND) ? O_APPEND : O_TRUNC);
            int fd = open(cmd->out_redir_file, flags, 0644);
            if (fd < 0) {
                perror("open output");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        execvp(cmd->argv[0], cmd->argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
        return OK;
    }
}

/*
 * Execute a pipeline of commands (with redirection)
 */
int execute_pipeline(command_list_t *clist) {
    if (clist->num == 1) {
        return exec_cmd(&clist->commands[0]);
    }

    int pipes[clist->num - 1][2];
    pid_t pids[clist->num];

    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
    }

    for (int i = 0; i < clist->num; i++) {
        pids[i] = fork();

        if (pids[i] < 0) {
            perror("fork");
            return ERR_EXEC_CMD;
        } else if (pids[i] == 0) { // Child process
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            if (i < clist->num - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            if (clist->commands[i].in_redir_type == REDIR_IN) {
                int fd = open(clist->commands[i].in_redir_file, O_RDONLY);
                if (fd < 0) {
                    perror("open input");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            if (clist->commands[i].out_redir_type != REDIR_NONE) {
                int flags = O_WRONLY | O_CREAT | ((clist->commands[i].out_redir_type == REDIR_APPEND) ? O_APPEND : O_TRUNC);
                int fd = open(clist->commands[i].out_redir_file, flags, 0644);
                if (fd < 0) {
                    perror("open output");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < clist->num; i++) {
        waitpid(pids[i], NULL, 0);
    }

    return OK;
}

/*
 * Handle built-in commands
 */
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    if (cmd->argc == 0) return BI_NOT_BI;

    Built_In_Cmds bi_cmd = match_command(cmd->argv[0]);
    
    switch (bi_cmd) {
        case BI_CMD_EXIT:
            printf("exiting...\n");
            return BI_CMD_EXIT;

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

        case BI_CMD_DRAGON:
            print_dragon();
            return BI_EXECUTED;

        default:
            return BI_NOT_BI;
    }
}

/*
 * match_command
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

