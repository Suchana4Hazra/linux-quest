/* sem1.c */
#include <stdio.h>
#include <sys/types.h> /* for semget(2) ftok(3) semop(2) semctl(2) */
#include <sys/ipc.h> /* for semget(2) ftok(3) semop(2) semctl(2) */
#include <sys/sem.h> /* for semget(2) semop(2) semctl(2) */
#include <unistd.h> /* for fork(2) */

#include <stdlib.h> /* for exit(3) */


#define NO_SEM  3
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <string.h>

void create_semaphore_set(key_t key, int num_sems) {
    int semid = semget(key, num_sems, IPC_CREAT | 0777);
    if (semid == -1) {
        perror("semget failed");
        return;
    }
    printf("Semaphore set created with ID: %d\n", semid);
}

void set_semaphore_value(int semid, int semnum, int semval) {
    if (semctl(semid, semnum, SETVAL, semval) == -1) {
        perror("semctl SETVAL failed");
        return;
    }
    printf("Semaphore %d set to %d\n", semnum, semval);
}

void get_semaphore_value(int semid, int semnum, int num_sems) {
    if (semnum >= 0) {
        int val = semctl(semid, semnum, GETVAL);
        if (val == -1) {
            perror("semctl GETVAL failed");
            return;
        }
        printf("Semaphore %d value: %d\n", semnum, val);
    } else {
	printf("No of semaphores: %d\n",num_sems);
        for (int i = 0; i < num_sems; i++) {
            int val = semctl(semid, i, GETVAL);
            if (val == -1) {
                perror("semctl GETVAL failed");
                return;
            }
            printf("Semaphore %d value: %d\n", i, val);
        }
    }
}

void increment_semaphore(int semid, int semnum, int val) {
    
    //semctl(semid, semnum, operation, arg)
    int prevVal = semctl(semid, semnum, GETVAL);
    int newVal = prevVal+val;
    semctl(semid, semnum, SETVAL, newVal);
    printf("Semaphore %d incremented by %d\n", semnum, val);
}

void decrement_semaphore(int semid, int semnum, int val) {
    
    int prevVal = semctl(semid, semnum, GETVAL);
    int newVal = prevVal-val;
    if(semctl(semid, semnum, SETVAL, newVal) == -1) {
        perror("Not Possible");
    } else {
        printf("Semaphore %d decremented by %d\n", semnum, val);
    }
}

void remove_semaphore_set(int semid) {
    if (semctl(semid, 0, IPC_RMID) == -1) { //In case of IPC_RMID semnum that is the 2nd parameter will be ignored
        perror("semctl IPC_RMID failed");
        return;
    }
    printf("Semaphore set removed\n");
}

void list_waiting_processes(int semid, int semnum, int num_sems) {
    if (semnum >= 0) {
        int waiting = semctl(semid, semnum, GETNCNT);
        if (waiting == -1) {
            perror("semctl GETNCNT failed");
            return;
        }
        printf("Semaphore %d: %d processes waiting.\n", semnum, waiting);
    } else {
        for (int i = 0; i < num_sems; i++) {
            int waiting = semctl(semid, i, GETNCNT);
            if (waiting == -1) {
                perror("semctl GETNCNT failed");
                return;
            }
            printf("Semaphore %d: %d processes waiting.\n", i, waiting);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <file_name> <project_id> <command> [args]\n", argv[0]);
        return 1;
    }

    key_t key = ftok(argv[1], atoi(argv[2]));
    if (key == -1) {
        perror("ftok failed");
        return 1;
    }

    if (strcmp(argv[3], "create") == 0 && argc == 5) {

        create_semaphore_set(key, atoi(argv[4]));

    }
    int semid = semget(key,0,0777);
    if(semid == -1) {
    
	    perror("Semget failed(semaphore set may not exist)");
	    return 1;
    }

    else if (strcmp(argv[3], "set") == 0 && argc == 6) {

        set_semaphore_value(semid, atoi(argv[4]), atoi(argv[5]));

    } else if (strcmp(argv[3], "get") == 0) {

        int semnum = (argc == 5) ? atoi(argv[4]) : -1;
        struct semid_ds sem_info;
	semctl(semid,0,IPC_STAT,&sem_info);
	int num_sems = sem_info.sem_nsems;
        get_semaphore_value(semid, semnum, num_sems);

    } else if (strcmp(argv[3], "inc") == 0 && argc == 6) {

        increment_semaphore(semid, atoi(argv[4]), atoi(argv[5]));

    } else if (strcmp(argv[3], "dcr") == 0 && argc == 6) {

        decrement_semaphore(semid, atoi(argv[4]), atoi(argv[5]));

    } else if (strcmp(argv[3], "rm") == 0) {

        remove_semaphore_set(semid);

    } else if (strcmp(argv[3], "listp") == 0) {

        int num_sems = semctl(semid, 0, GETVAL);
        int semnum = (argc == 5) ? atoi(argv[4]) : -1;
        list_waiting_processes(semid, semnum, num_sems);

    } else {
        fprintf(stderr, "Invalid command\n");
        return 1;
    }

    return 0;
}

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
