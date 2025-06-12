#include "systemcalls.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    int ret = system(cmd);
    return (ret != -1 && WIFEXITED(ret) && WEXITSTATUS(ret) == 0);
}

/**
 * @param count -The numbers of variables passed to the function.
 *   followed by arguments to pass to the command
 * @param ... - A list of 1 or more arguments after the @param count argument.
 * @return true if the command executed successfully with execv(), false otherwise
 */
bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count + 1];
    for (int i = 0; i < count; i++) {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    va_end(args);

    pid_t pid = fork();
    if (pid == -1) {
        return false;
    } else if (pid == 0) {
        execv(command[0], command);
        perror("execv");
        exit(1);
    }

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        return false;
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

/**
 * @param outputfile - The full path to the file to write with command output.
 * All other parameters, see do_exec above
 */
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count + 1];
    for (int i = 0; i < count; i++) {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    va_end(args);

    pid_t pid = fork();
    if (pid == -1) {
        return false;
    } else if (pid == 0) {
        int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open");
            exit(1);
        }

        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            close(fd);
            exit(1);
        }

        close(fd);

        execv(command[0], command);
        perror("execv");
        exit(1);
    }

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        return false;
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

