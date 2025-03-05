#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include<signal.h>

int shmid;
struct task *solve;
struct task {
        char data[100];
        pid_t worker_pid;
        int status;
};

int main() {

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

    while(1) {

	    while(solve->status != 1);

	    if(solve->status == -1) {
		    printf("\n Worker process having pid: %d has terminated", getpid());
		    shmdt(solve);
	    }

	    if(solve->status == 1) {
	         solve->status = 2;
	         solve->worker_pid = getpid();
		 int length = strlen(solve->data);
		 printf("Length is calculated by process: %d\n",getpid());
		 printf("Calculated length: %d\n",length);
		 solve->data[0] = length;
	    }
           solve->status = 3;

	   sleep(1);
    }
}
