/* sem1.c */
#include <stdio.h>
#include <sys/types.h> /* for semget(2) ftok(3) semop(2) semctl(2) */
#include <sys/ipc.h> /* for semget(2) ftok(3) semop(2) semctl(2) */
#include <sys/sem.h> /* for semget(2) semop(2) semctl(2) */
#include <unistd.h> /* for fork(2) */

#include <stdlib.h> /* for exit(3) */


#define NO_SEM	1

#define P(s) semop(s, &Pop, 1);
#define V(s) semop(s, &Vop, 1);



struct sembuf Pop;
struct sembuf Vop;



int main() {
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

	setvalArg.val = 1;


	/* struct sembuf has the following fields */
	//unsigned short sem_num;  /* semaphore number */
        //short          sem_op;   /* semaphore operation */
        //short          sem_flg;  /* operation flags */

	Pop.sem_num = 0;
	Pop.sem_op = -1;
	Pop.sem_flg = SEM_UNDO;

	Vop.sem_num = 0;
	Vop.sem_op = 1;
	Vop.sem_flg = SEM_UNDO;

	// key_t ftok(const char *pathname, int proj_id);
	mykey = ftok("/home/manas/Documents/Operating Systems/samples/sem/sem1.c", 1);
	
	if (mykey == -1) {
		perror("ftok() failed");
		exit(1);
	}

	//       int semget(key_t key, int nsems, int semflg);
	semid = semget(mykey, NO_SEM, IPC_CREAT | 0777);
	if(semid == -1) {
		perror("semget() failed");
		exit(1);
	}

	// int semctl(int semid, int semnum, int cmd, ...);
	status = semctl(semid, 0, SETVAL, setvalArg);
	if(status == -1) {
		perror("semctl() failed");
		exit(1);
	}

	pid = fork();
	if (pid == -1) {
		perror("fork() failed");
		exit(1);
	}

	if (pid == 0) {
		/* child process task */
		P(semid);
		fprintf(stdout, "Child.\n");
		V(semid);
	} else {
		/* parent process task */
		P(semid);
		fprintf(stdout, "Parent.\n");
		V(semid);
	}

}
