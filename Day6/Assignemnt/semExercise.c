/* sem1.c */
#include <stdio.h>
#include <sys/types.h> /* for semget(2) ftok(3) semop(2) semctl(2) */
#include <sys/ipc.h> /* for semget(2) ftok(3) semop(2) semctl(2) */
#include <sys/sem.h> /* for semget(2) semop(2) semctl(2) */
#include <unistd.h> /* for fork(2) */

#include <stdlib.h> /* for exit(3) */


#define NO_SEM  3

#define P(s) semop(s, &Pop, 1);
#define V(s) semop(s, &Vop, 1);



struct sembuf Pop;
struct sembuf Vop;

int main(int argc, int *argv[]) {

	if(argc < 6) {
		printf("Invalid Arguments\n");
		return 0;
	}

        key_t mykey;
        pid_t pid;

        int semid;

        int status;

        union semun {
                int              val;    /* Value for SETVAL */
                struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
                unsigned short  *array;  /* Array for GETALL, SETALL */
                struct seminfo  *__buf;  /* Buffer for IPC_INFO (Linux-specific) */
        } setvalArg;


        /* struct sembuf has the following fields */
        //unsigned short sem_num;  /* semaphore number 
        //short          sem_op;   /* semaphore operation 
        //short          sem_flg;  /* operation flags 

	// key_t ftok(const char *pathname, int proj_id);
        mykey = ftok(argv[1], argv[2]);

        if (mykey == -1) {
                perror("ftok() failed");
                exit(1);
        }

	if(strcmp(argv[3],"create") == 0) {
        //       int semget(key_t key, int nsems, int semflg);
        semid = semget(mykey, atoi(argv[4]), IPC_CREAT | 0777);
        if(semid == -1) {
                perror("semget() failed");
                exit(1);
        } else {
	     printf("\nSuccessfully created Semaphore set with semid: %d",semid);
	}

	} else if(strcmp(argv[3],"set") == 0) {
	     
	
           setvalArg.val = argv[5];
	   int status = semctl(semid, atoi(argv[4]), SETVAL, setvalArg);
           if(status == -1) {
	      perror("semctl() failed\n");
	      exit(1);
	  }	   
	} else if(strcmp(argv[3],"get") == 0) {

		if(argv[4] == NULL) {
		    

		}
		int status = semctl(semid,atoi(argv[4]), GETVAL);
	        if(status == -1) {
			perror("semctl() failed\n");
                        exit(1);
		}

		printf("\n The value of the semaphore: %d is %d,atoi(argv[4]),status);	
	        	
	}
}
