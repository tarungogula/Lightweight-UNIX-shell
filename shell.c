#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1024           // Maximum size for user input
#define MAX_TOKEN_SIZE 64             // Maximum size for each token
#define MAX_NUM_TOKENS 64             // Maximum number of tokens in a command
#define MAX_NUM_BACKGROUND 64         // Maximum number of background processes

int p = -1;  // Global variable to store the process group ID of the foreground process

// Function to split the input line into tokens (command and arguments)
char **tokenize(char *line) {
    char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
    char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
    int i, tokenIndex = 0, tokenNo = 0;

    // Loop through the input line and tokenize it based on spaces, newlines, or tabs
    for (i = 0; i < strlen(line); i++) {
        char readChar = line[i];

        if (readChar == ' ' || readChar == '\n' || readChar == '\t') {
            token[tokenIndex] = '\0';
            if (tokenIndex != 0) {
                tokens[tokenNo] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
                strcpy(tokens[tokenNo++], token);
                tokenIndex = 0;
            }
        } else {
            token[tokenIndex++] = readChar;
        }
    }

    free(token);
    tokens[tokenNo] = NULL;  // Null-terminate the array of tokens
    return tokens;
}

// Function to reap finished background processes and clean up zombie processes
int background_process_reaping(int a[], int l) {
    int i, b[MAX_NUM_BACKGROUND], id, k = 0;

    for (i = 0; i < l; i++) {
        id = waitpid(a[i], NULL, WNOHANG);
        if (id != 0) {
            printf("Shell: Background process finished\n");
        } else {
            b[k] = a[i];
            k = k + 1;
        }
    }
    for (i = 0; i < k; i++) {
        a[i] = b[i];
    }
    return k;
}

// Function to change the current directory (cd command)
void change_directory(char* tokens[]) {
    if (tokens[2] != NULL) {
        printf("Shell: Incorrect command\n");
    } else {
        int x = chdir(tokens[1]);
        if (x != 0) {
            printf("Shell: Incorrect command\n");
        }
    }
}

// Function to handle the exit command, which terminates all background processes
void exit_command(int a[], int l) {
    int i, id;
    for (i = 0; i < l; i++) {
        id = waitpid(a[i], NULL, WNOHANG);
        if (id <= 0) {
            kill(a[i], SIGINT);  // Send SIGINT to terminate the process
        }
    }
    return;
}

// Function to handle SIGINT (Ctrl+C) by terminating the foreground process
void signal_interrupt() {
    kill(-p, SIGINT);  // Send SIGINT to the process group of the foreground process
    return;
}

int main(int argc, char* argv[]) {
    char line[MAX_INPUT_SIZE];            // Buffer to store user input
    char **tokens;                        // Array to store command tokens
    int i, bg = 0, bgp[MAX_NUM_BACKGROUND], j = 0;

    // Set up signal handler for SIGINT (Ctrl+C)
    signal(SIGINT, signal_interrupt);

    while (1) {
        j = background_process_reaping(bgp, j);
        bzero(line, sizeof(line));  // Clear the input buffer
        printf("$ ");
        scanf("%[^\n]", line);      // Read a line of input from the user
        getchar();                  // Consume the newline character

        bg = 0;
        int a = strlen(line) - 1;
        if ((line[a]) == '&') {     // Check if the command should run in the background
            if (j == 64) {
                printf("Can't create background process\n");
            } else {
                bg = 1;
                line[a - 1] = '\n';
                line[a] = '\0';
            }
        } else {
            line[strlen(line)] = '\n';
        }

        tokens = tokenize(line);    // Tokenize the input line

        if (tokens[0] == NULL) {    // Ignore empty input
            continue;
        }

        if (strcmp(tokens[0], "cd") == 0) {  // Handle the 'cd' command
            change_directory(tokens);
        } else if (strcmp(tokens[0], "exit") == 0) {  // Handle the 'exit' command
            exit_command(bgp, j);
            for (i = 0; tokens[i] != NULL; i++) {
                free(tokens[i]);
            }
            free(tokens);
            return 0;
        } else {
            int fc = fork();  // Create a new child process
            if (fc < 0) {
                fprintf(stderr, "%s\n", "Unable to create child process\n");
            } else if (fc == 0) {  // Child process
                execvp(tokens[0], tokens);
                printf("Command execution failed\n");
                exit(1);
            } else {  // Parent process
                if (bg == 0) {  // Foreground process
                    setpgid(fc, fc);
                    p = getpgid(fc);
                    waitpid(fc, NULL, 0);
                } else {  // Background process
                    setpgid(fc, fc);
                    bgp[j] = fc;
                    j = j + 1;
                    printf("[%d]\t%d\n", j, fc);
                }
            }
        }

        // Free allocated memory for tokens
        for (i = 0; tokens[i] != NULL; i++) {
            free(tokens[i]);
        }
        free(tokens);
    }
    return 0;
}
