/*This is a sample program involving  2 processes (parent and a child) using a shared integer variable.  The parent process goes on putting numbers (given by the user) in the shared integer variable and the child process simply goes on printing that integer variable.

"Signal-Handler" might be a new programming idea introduced in this program. For comprehending what "Signal-Handler" is, please read the example stated in the next paragraph. (You should also read the manual pages referred to in the program given below. 

Normally, if ctrl-c (^C) keys are pressed at the keyboard, the currently executing process gets terminated. In the program given below, a "Signal-Handler" is added, so that if ctrl-c (^C) keys are  pressed during execution of this program, a user-defined function is called which after releasing shared memory kills the process(es).
*/

#include <stdio.h>
#include <unistd.h> /* for fork() */
#include <sys/wait.h> /* for wait() */
#include <sys/types.h> /* for wait() kill(2)*/
#include <sys/ipc.h> /* for shmget() shmctl() */
#include <sys/shm.h> /* for shmget() shmctl() */

#include <signal.h> /* for signal(2) kill(2) */

#include <errno.h> /* for perror */

#include <stdlib.h>



int shmid; /* stores the id of the  shared memory segment as returned by shmget(). this variable is defined globally so that signal handler can access it for releasing the shared memory segment. */

/* following is a signal handler for the keypress ^C, that is, ctrl-c */
typedef void (*sighandler_t)(int);
void releaseSHM(int signum) {
        int status;
        // int shmctl(int shmid, int cmd, struct shmid_ds *buf); /* Read the manual for shmctl() */
        status = shmctl(shmid, IPC_RMID, NULL); /* IPC_RMID is the command for destroying the shared memory*/
        if (status == 0) {
                fprintf(stderr, "Remove shared memory with id = %d.\n", shmid);
        } else if (status == -1) {
                fprintf(stderr, "Cannot remove shared memory with id = %d.\n", shmid);
        } else {
                fprintf(stderr, "shmctl() returned wrong value while removing shared memory with id = %d.\n", shmid);
        }

        // int kill(pid_t pid, int sig);
        status = kill(0, SIGKILL);
        if (status == 0) {
                fprintf(stderr, "kill susccesful.\n"); /* this line may not be executed :P WHY?*/
        } else if (status == -1) {
                perror("kill failed.\n");
                fprintf(stderr, "Cannot remove shared memory with id = %d.\n", shmid);
        } else {
                fprintf(stderr, "kill(2) returned wrong value.\n");
        }
}
                                                                                                                             1,1           Top

int main() {
        int status;
        pid_t pid=0;
        pid_t p1=0;

        sighandler_t shandler;


        /* install signal handler */
        // sighandler_t signal(int signum, sighandler_t handler);
        shandler =  signal(SIGINT, releaseSHM); /* should we call this seperately in parent and child process */



        // int shmget(key_t key, size_t size, int shmflg);

        shmid =  shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0777);

        if (shmid == -1) { /* shmget() failed() */
                perror("shmget() failed: ");
                exit (1);
        }

        printf("shmget() returns shmid = %d.\n", shmid);

        p1 = fork();
        if (p1 == 0) { /* child process */

                int i;
                int *pi_child;

                // void *shmat(int shmid, const void *shmaddr, int shmflg);
                pi_child = shmat(shmid, NULL, 0);
                if (pi_child == (void *)-1) { /* shmat fails */
                        perror("shmat() failed at child: ");
                        exit (1);
                }

                for (i=0; i<50; i++) {
                        printf("Child Reads %d.\n", *pi_child);
                        getchar();
                                                                                                                             84,4-25       60%
                }
                exit(0);

        } else { /* parent process */
                int i;
                int *pi_parent;

                // void *shmat(int shmid, const void *shmaddr, int shmflg);
                pi_parent = shmat(shmid, NULL, 0);
                if (pi_parent == (void *)-1) { /* shmat fails */
                        perror("shmat() failed at parent: ");
                        exit (1);
                }

                for (i=0; i<50; i++) {
                        *pi_parent = i;
                        printf("Parent writes %d.\n", *pi_parent);
                        getchar();
                }
                pid = wait(&status);
                printf("pid = %d status = %d!\n", pid, status);
                exit(0);
        }    
    }