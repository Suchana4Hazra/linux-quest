/*

Let there be a (parent) process which goes on putting a random (integer) number  in a shared variable, say n,  in a regular interval. There is a child process that goes on putting the factorial of the integer (that it finds in n) back into the shared variable (n), in regular intervals. The parent process prints the factorial that it receives from the child process. Device your own mechanism(s) so that the child process "knows" that a new number has been put in n  by the parent process and the parent process "knows" that a the factorial of a number has been put in n  by the child  process,

Write a complete C program to implement the above. The program should make arrangements for releasing shared memory that has been created during execution (as demonstrated in the sample program).

The processes must print meaningful output so that the user understands what is happening.

Note:

    For generation for random numbers you may use the "rand()" function. "man 3 rand" will show you the manual.
    Since the parent and child processes run independently at their own speed, there may be inconsistency in the results printed by them. For example, the parent process may manage to put numbers in a  faster manner than the child process can handle. You may even try to demonstrate that such inconsistency appears in the output. How to avoid such inconsistencies will be discussed later. 
    */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

int shmid; // Global variable for shared memory ID

// Signal handler to clean up shared memory
void releaseSHM(int signum) {
    if (shmctl(shmid, IPC_RMID, NULL) == 0) {
        fprintf(stderr, "Shared memory (id=%d) removed.\n", shmid);
    } else {
        perror("shmctl failed");
    }
    exit(0);
}

// Function to compute factorial
long long factorial(int n) {
    if (n < 0) return -1; // Invalid case
    long long fact = 1;
    for (int i = 1; i <= n; i++)
        fact *= i;
    return fact;
}

int main() {
    signal(SIGINT, releaseSHM); // Handle Ctrl+C to clean shared memory

    // Create shared memory
    shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0777); // Single integer for shared memory
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }
    
    printf("Shared memory ID = %d\n", shmid);

    // Fork a child process
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    }

    // Attach shared memory
    int *sharedMem = (int *)shmat(shmid, NULL, 0);
    if (sharedMem == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    if (pid == 0) { // Child Process
        while (1) {
            sleep(rand() % 3); // Random delay
            int num = *sharedMem;
            printf("[Child] Reads %d from shared memory.\n", num);
            *sharedMem = factorial(num); // Store factorial in shared memory
            printf("[Child] Computes factorial %lld.\n", (long long)*sharedMem);
        }
    } else { // Parent Process
        srand(time(NULL)); // Seed random number generator
        for (int i = 0; i < 10; i++) {
            sleep(rand() % 3); // Random delay
            *sharedMem = rand() % 10 + 1; // Generate random number (1 to 10)
            printf("[Parent] Writes %d to shared memory.\n", *sharedMem);
        }

        kill(pid, SIGKILL); // Kill child process
        wait(NULL); // Wait for child to terminate
        releaseSHM(0); // Cleanup shared memory
    }

    return 0;
}
