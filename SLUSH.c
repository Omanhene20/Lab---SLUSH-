#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_INPUT 256
#define MAX_ARGS 15

void handle_sigint(int sig) {
    printf("\nslush> ");
    fflush(stdout);
}

// Function to execute a single command (without pipelines)
void execute_command(char *cmd) {
    char *args[MAX_ARGS + 1];
    int i = 0;
    
    char *token = strtok(cmd, " ");
    while (token != NULL && i < MAX_ARGS) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL; // Null-terminate argument list

    if (args[0] == NULL) return; // Empty command

    // Handle built-in 'cd' command
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "cd: Missing directory\n");
        } else if (chdir(args[1]) != 0) {
            perror("cd");
        }
        return;
    }

    // Fork and execute external command
    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args);
        perror(args[0]); // Print error if execvp fails
        exit(1);
    } else if (pid > 0) {
        wait(NULL); // Parent waits for child
    } else {
        perror("fork");
    }
}

// Recursive function to handle pipelining
void execute_pipeline(char *cmd) {
    char *right = strchr(cmd, '('); // Find the pipeline symbol
    if (right) {
        *right = '\0'; // Split into two commands
        right++; // Move past '('

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            return;
        }

        pid_t pid = fork();
        if (pid == 0) { 
            // Child: Executes left command
            close(pipefd[0]); // Close read end
            dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to pipe
            close(pipefd[1]);
            execute_command(cmd);
            exit(0);
        } else if (pid > 0) {
            // Parent: Executes right command
            close(pipefd[1]); // Close write end
            dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to pipe
            close(pipefd[0]);
            execute_pipeline(right);
            wait(NULL); // Wait for child
        } else {
            perror("fork");
        }
    } else {
        execute_command(cmd);
    }
}

int main() {
    char input[MAX_INPUT];

    signal(SIGINT, handle_sigint); // Handle Ctrl+C

    while (1) {
        printf("slush> ");
        fflush(stdout);

        if (fgets(input, MAX_INPUT, stdin) == NULL) {
            printf("\n");
            break; // Exit on EOF (^D)
        }

        input[strcspn(input, "\n")] = 0; // Remove newline

        if (strlen(input) > 0) {
            execute_pipeline(input);
        }
    }

    return 0;
}
