# Assignment Questions and Answers

## 1. Your shell forks multiple child processes when executing piped commands. How does your implementation ensure that all child processes complete before the shell continues accepting user input? What would happen if you forgot to call waitpid() on all child processes?

### **Answer:**
My shell ensures that **all child processes complete** before accepting new input by:
1. **Forking a new process for each piped command** and storing the PIDs.
2. **Closing unused pipe ends** after duplicating the necessary file descriptors.
3. **Calling `waitpid()` on all child processes** in a loop to ensure they finish.

If `waitpid()` is **not** called on all child processes:
- The processes would become **zombie processes** (defunct processes still in the process table).
- The parent shell would **immediately return to the prompt**, even if the commands haven’t finished executing.
- **Pipes may not close properly**, leading to unexpected behavior like blocked reads.

---

## 2. The dup2() function is used to redirect input and output file descriptors. Explain why it is necessary to close unused pipe ends after calling dup2(). What could go wrong if you leave pipes open?

### **Answer:**
**Closing unused pipe ends after `dup2()` is necessary to prevent resource leaks and deadlocks.**  
When a child process redirects input/output with `dup2()`, the original pipe file descriptors remain open. If we **do not close them**:
- **The shell may hang indefinitely**: The reading process waits for EOF, but since unused write ends are still open, EOF **never arrives**.
- **Resource exhaustion**: Too many open file descriptors can cause the system to **run out of available pipes**.
- **Confusing debugging**: Commands may exhibit inconsistent behavior because pipes are left open unexpectedly.

Proper usage:
```c
dup2(pipes[i - 1][0], STDIN_FILENO); // Redirect input
close(pipes[i - 1][0]); // Close unused read end

