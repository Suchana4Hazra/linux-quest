#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_SIZE 100
#define MAX_COMMAND_LENGTH 1000

// Function to parse a command into arguments
void parse(char *command, char **args) {
    char *token = strtok(command, " ");
    int i = 0;
    while (token != NULL && i < MAX_SIZE - 1) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL; // Null-terminate the array of arguments
}

// Function to execute internal commands
int executeInternalCommand(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "mysh: cd: missing argument\n");
        } else if (chdir(args[1]) != 0) {
            perror("mysh: cd");
        }
        return 1;
    } else if (strcmp(args[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("mysh: pwd");
        }
        return 1;
    } else if (strcmp(args[0], "clear") == 0) {
        system("clear");
        return 1;
    } else if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }
    return 0; // Not an internal command
}

// Function to execute external commands
void executeExternalCommand(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("mysh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("mysh: fork failed");
    } else {
        // Parent process
        wait(NULL);
    }
}

// Function to handle multiple commands connected by ;, &&, ||, and |
void handleMultipleCommands(char *command) {
    char *subcommands[MAX_SIZE];
    char *delimiters = ";|&";
    char *token = strtok(command, delimiters);
    int i = 0;

    while (token != NULL) {
        subcommands[i++] = token;
        token = strtok(NULL, delimiters);
    }
    subcommands[i] = NULL;

    for (int j = 0; subcommands[j] != NULL; j++) {
        char *args[MAX_SIZE];
        parse(subcommands[j], args);

        if (args[0] == NULL) {
            continue; // Skip empty commands
        }

        if (!executeInternalCommand(args)) {
            executeExternalCommand(args);
        }
    }
}


int main(int argc, char *argv[]) {
    
    do {
        printf("\nmysh:$ ");
        char command[MAX_COMMAND_LENGTH];
        fgets(command, sizeof(command), stdin);

        // Remove newline character
        command[strcspn(command, "\n")] = 0;

        // Handle multiple commands
        handleMultipleCommands(command);

    } while (1);

    return 0;
}
