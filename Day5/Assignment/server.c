#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

int shmid;
struct task *solve;
struct task {
	char data[100];
	pid_t worker_pid;
	int status;
};

void releaseSHM(int signum) {
  printf("Terminated\n");
  solve->status = -1;
  shmdt(solve);
}
int main() {

     signal(SIGINT, releaseSHM);
    // Generate Key
    key_t SHM_KEY = ftok("keyFile",65);

    // Create shared memory segment
    shmid = shmget(SHM_KEY, sizeof(struct task), IPC_CREAT | 0777);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }

    // Attach to shared memory
    solve = (struct task *)shmat(shmid, NULL, 0);
    if (solve == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    // Initialize shared memory
    solve->status = 0;

    while (1) {
        // Generate a random string
        int len = rand() % 10 + 5;  // String length between 5 and 14
        for (int i = 0; i < len; i++)
            solve->data[i] = 'a' + rand() % 26;
        solve->data[len] = '\0';

        printf("Server: Generated string = %s\n", solve->data);
        solve->status = 1; // Indicate new data is available

        // Wait for a worker to compute the length
        while (solve->status != 3);

        // Read computed length
        printf("Server: Worker %d computed length = %d\n", solve->worker_pid, (int)solve->data[0]);
        solve->status = 4; // Indicate server has used the result
	sleep(2);
    }

    return 0;
}
