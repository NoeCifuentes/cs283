1. In this assignment I suggested you use `fgets()` to get user input in the main while loop. Why is `fgets()` a good choice for this application?

    > **Answer**: `fgets()` prevents buffer overflow, reads input line-by-line, handles EOF (Ctrl+D) gracefully, and ensures null-termination.

2. You needed to use `malloc()` to allocate memory for `cmd_buff` in `dsh_cli.c`. Can you explain why you needed to do that, instead of allocating a fixed-size array?

    > **Answer**: `malloc()` allows dynamic memory allocation, making the shell more flexible and avoiding wasted or insufficient memory.

3. In `dshlib.c`, the function `build_cmd_list()` must trim leading and trailing spaces from each command before storing it. Why is this necessary? If we didn't trim spaces, what kind of issues might arise when executing commands in our shell?

    > **Answer**: Trimming prevents command misinterpretation, unrecognized executables, and empty command entries from extra spaces.

4. For this question you need to do some research on STDIN, STDOUT, and STDERR in Linux. We've learned this week that shells are "robust brokers of input and output". Google _"linux shell stdin stdout stderr explained"_ to get started.

- One topic you should have found information on is "redirection". Please provide at least 3 redirection examples that we should implement in our custom shell, and explain what challenges we might have implementing them.

    > **Answer**:  
    > 1. `ls > file.txt` (Redirect output to a file) – Requires handling `stdout` redirection.  
    > 2. `sort < input.txt` (Read input from a file) – Requires replacing `stdin`.  
    > 3. `gcc code.c 2> errors.log` (Redirect errors) – Requires handling `stderr` separately.

- You should have also learned about "pipes". Redirection and piping both involve controlling input and output in the shell, but they serve different purposes. Explain the key differences between redirection and piping.

    > **Answer**: Redirection transfers data between a command and a file, while piping connects processes dynamically (`cmd1 | cmd2`).

- STDERR is often used for error messages, while STDOUT is for regular output. Why is it important to keep these separate in a shell?

    > **Answer**: Separating `stderr` ensures error messages don’t mix with standard output, allowing better debugging and selective redirection.

- How should our custom shell handle errors from commands that fail? Consider cases where a command outputs both STDOUT and STDERR. Should we provide a way to merge them, and if so, how?

    > **Answer**: Detect non-zero exit codes, allow `stderr` redirection (`2>`), and optionally merge `stdout` and `stderr` using `command > output.txt 2>&1`.

