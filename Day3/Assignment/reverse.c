/*Write a complete C program that reads n strings as command line arguments.
 That is, uses "int argc" and "char *argv[]" to read S1, S2, ..., Sn, 
 when the program is executed as "./a.out S1 S2 ... Sn". 
 The program then creates n child processes P1, P2, ..., Pn such that Pi, 1 ≤  i ≤  n, 
 reverses the string Si and prints the reversed string.*/
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

// Function to reverse a string
void reverseString(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = temp;
    }
}

int main(int argc, char *argv[]) {
    // Check if there are enough arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: %s S1 S2 ... Sn\n", argv[0]);
        return 1;
    }

    int n = argc - 1; // Number of strings passed
    pid_t pid;

    for (int i = 1; i <= n; i++) {
        pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) { // Child process
            char *str = argv[i];
            reverseString(str);
            printf("Reversed string from process %d: %s\n", getpid(), str);
            exit(0); // Exit child process
        }
    }

    // Parent process waits for all children to finish
    /*for (int i = 1; i <= n; i++) {
        wait(NULL);
    }*/

    return 0;
}

