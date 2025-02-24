# Assignment Questions - Shell Part 2

## 1. Why use fork/execvp instead of just execvp? What value does fork provide?
**Answer:** `fork()` allows the parent process to continue running while the child process executes the new program. Without `fork()`, `execvp()` would replace the current shell, making it impossible to return to the shell after executing a command.

## 2. What happens if fork() fails? How does your implementation handle this?
**Answer:** If `fork()` fails, it returns `-1`, indicating no new process was created. My implementation checks this return value and prints an error message (`perror("fork")`), returning an error code instead of proceeding.

## 3. How does execvp() find the command? What system environment variable is used?
**Answer:** `execvp()` searches for the command in directories listed in the `PATH` environment variable. It checks each directory for an executable matching the given command name.

## 4. Why call wait() in the parent after forking? What happens if we don’t?
**Answer:** `wait()` ensures the parent waits for the child process to complete, preventing zombie processes. Without `wait()`, the child process might become a zombie, leading to resource leaks.

## 5. What does WEXITSTATUS() do? Why is it important?
**Answer:** `WEXITSTATUS(status)` extracts the exit code of the child process from the `wait()` return status. This helps determine if the command executed successfully or failed.

## 6. How does build_cmd_buff() handle quoted arguments? Why is this necessary?
**Answer:** It treats quoted strings as a single argument, preserving spaces inside quotes. This prevents unintended argument splitting, ensuring commands like `echo "hello world"` work correctly.

## 7. Changes to parsing logic compared to the previous assignment. Challenges?
**Answer:** I removed pipe parsing and adapted the logic to use `cmd_buff_t` instead of `command_list_t`. Handling quoted arguments was tricky due to edge cases like escaped quotes inside strings.

## 8. What is the purpose of signals in Linux? How do they differ from IPC?
**Answer:** Signals notify processes of events (e.g., termination, suspension). Unlike other IPC methods (e.g., shared memory, pipes), signals are asynchronous and system-managed.

## 9. Three commonly used signals and their use cases?
**Answer:**
- **SIGKILL (9)**: Forcefully terminates a process; cannot be caught or ignored.
- **SIGTERM (15)**: Gracefully requests a process to terminate; can be handled.
- **SIGINT (2)**: Sent when a user presses `Ctrl+C`; typically stops a process.

## 10. What happens when a process receives SIGSTOP? Can it be ignored like SIGINT?
**Answer:** `SIGSTOP` suspends a process until resumed with `SIGCONT`. It **cannot** be caught or ignored, unlike `SIGINT`, because it is directly handled by the kernel.

