#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

int shmid; // Shared memory ID

// Function to compute a row of the product matrix
void computeRow(int row, int n, int m, int p, int A[n][m], int B[m][p], int C[n][p]) {
    for (int j = 0; j < p; j++) {
        C[row][j] = 0; // Initialize to 0
        for (int k = 0; k < m; k++) {
            C[row][j] += A[row][k] * B[k][j]; // Multiply and accumulate
        }
    }
}

void releaseSHM(int signum) {
    // Clean up shared memory
    if (shmctl(shmid, IPC_RMID, NULL) == 0) {
        fprintf(stderr, "Shared memory (id=%d) removed.\n", shmid);
    } else {
        perror("shmctl failed");
    }
    exit(0);
}

int main() {
    int n, m, p;

    // Read matrix dimensions
    printf("Enter number of rows for matrix A (n): ");
    scanf("%d", &n);
    printf("Enter number of columns for matrix A / rows for matrix B (m): ");
    scanf("%d", &m);
    printf("Enter number of columns for matrix B (p): ");
    scanf("%d", &p);

    // Declare matrices A, B, and C
    int A[n][m], B[m][p];

    // Read matrix A
    printf("Enter elements of matrix A (n x m):\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            scanf("%d", &A[i][j]);
        }
    }

    // Read matrix B
    printf("Enter elements of matrix B (m x p):\n");
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < p; j++) {
            scanf("%d", &B[i][j]);
        }
    }

    // Create shared memory for matrix C
    shmid = shmget(IPC_PRIVATE, sizeof(int) * n * p, IPC_CREAT | 0777);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }

    // Attach shared memory
    int (*C)[p] = (int (*)[p]) shmat(shmid, NULL, 0);
    if (C == (void *) -1) {
        perror("shmat failed");
        exit(1);
    }

    // Signal handler to clean up shared memory when the program ends
    signal(SIGINT, releaseSHM);

    // Create child processes to compute rows of the product matrix C
    pid_t pid;
    for (int i = 0; i < n; i++) {
        pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        }

        if (pid == 0) {  // Child process
            computeRow(i, n, m, p, A, B, C);
            printf("Child %d finished computing row %d\n", getpid(), i);
            exit(0);
        }
    }

    // Parent process waits for all child processes to finish
    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    // Print the result matrix C
    printf("Product Matrix C (n x p):\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < p; j++) {
            printf("%d ", C[i][j]);
        }
        printf("\n");
    }

    // Clean up shared memory
    releaseSHM(0);

    return 0;
}
