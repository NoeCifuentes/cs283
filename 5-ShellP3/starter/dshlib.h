#ifndef __DSHLIB_H__
#define __DSHLIB_H__

// Constants for command structure sizes
#define EXE_MAX 64
#define ARG_MAX 256
#define CMD_MAX 8
#define CMD_ARGV_MAX (CMD_MAX + 1)

// Longest command that can be read from the shell
#define SH_CMD_MAX (EXE_MAX + ARG_MAX)

// Redirection types for input/output handling
typedef enum {
    REDIR_NONE = 0,
    REDIR_IN,         // <
    REDIR_OUT,        // >
    REDIR_APPEND      // >>
} RedirectionType;

// Structure for storing command information
typedef struct cmd_buff {
    int argc;
    char *argv[CMD_ARGV_MAX];
    char *_cmd_buffer;

    // Redirection fields (for extra credit)
    RedirectionType in_redir_type;
    char *in_redir_file;
    RedirectionType out_redir_type;
    char *out_redir_file;
} cmd_buff_t;

// Structure for handling multiple commands in a pipeline
typedef struct command_list {
    int num;
    cmd_buff_t commands[CMD_MAX];
} command_list_t;

// Special character defines
#define SPACE_CHAR  ' '
#define PIPE_CHAR   '|'
#define PIPE_STRING "|"
#define REDIR_IN_CHAR '<'
#define REDIR_OUT_CHAR '>'

// Shell prompt and built-in commands
#define SH_PROMPT "dsh3> "
#define EXIT_CMD "exit"

// Standard Return Codes
#define OK                       0
#define WARN_NO_CMDS            -1
#define ERR_TOO_MANY_COMMANDS   -2
#define ERR_CMD_OR_ARGS_TOO_BIG -3
#define ERR_CMD_ARGS_BAD        -4
#define ERR_MEMORY              -5
#define ERR_EXEC_CMD            -6
#define OK_EXIT                 -7

// Output messages
#define CMD_OK_HEADER       "PARSED COMMAND LINE - TOTAL COMMANDS %d\n"
#define CMD_WARN_NO_CMD     "warning: no commands provided\n"
#define CMD_ERR_PIPE_LIMIT  "error: piping limited to %d commands\n"
#define CMD_ERR_REDIR       "error: redirection syntax error\n"

// Built-in command enum
typedef enum {
    BI_CMD_EXIT,
    BI_CMD_DRAGON,
    BI_CMD_CD,
    BI_NOT_BI,
    BI_EXECUTED,
} Built_In_Cmds;

// Function prototypes
int alloc_cmd_buff(cmd_buff_t *cmd_buff);
int free_cmd_buff(cmd_buff_t *cmd_buff);
int clear_cmd_buff(cmd_buff_t *cmd_buff);
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff);
int close_cmd_buff(cmd_buff_t *cmd_buff);
int build_cmd_list(char *cmd_line, command_list_t *clist);
int free_cmd_list(command_list_t *cmd_lst);

// Built-in command functions
Built_In_Cmds match_command(const char *input); 
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd);

// Main execution functions
int exec_local_cmd_loop();
int exec_cmd(cmd_buff_t *cmd);
int execute_pipeline(command_list_t *clist);

#endif // __DSHLIB_H__

