/*Write a complete C program that reads for an array of n numbers (value of n is given by the user). The program then creates n child processes P1, P2, ..., Pn such that Pi, 1 ≤  i ≤  n, computes the ith largest number from the array and passes it to the parent process (say, P) through "exit(status)" system call and terminates. ["man 3 exit" shows the manual page.] 

Note that through wait() or waitpid() systems calls, the parent process may get the value of the status with which a child process has exited with.

In this case, P receives ith largest number from Pi, 1 ≤  i ≤  n and builds a sorted array.   P, then, prints the numbers in descending order from that sorted array.
*/
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/wait.h>

void sort(int arr[],int n) {

        //SELECTION SORT ALGO
        for(int i=0;i<n;i++) {
                int maxIndex = i;
                for(int j=i+1;j<n;j++) {
                        if(arr[j] > arr[maxIndex]) {
                               maxIndex = j;
                        }
                }

        int temp = arr[i];
        arr[i] = arr[maxIndex];
        arr[maxIndex] = temp;
        }
}
int main() {

        int n;
        printf("Enter how many numbers? ");
        scanf("%d",&n);

        int arr[n];
        printf("Enter numbers one by one: \n");
        for(int i=0;i<n;i++) {

                scanf("%d",&arr[i]);
        }
        printf("Entered numbers........\n");
        for(int i=0;i<n;i++) {
                printf("%d  ",arr[i]);
        }
        sort(arr,n);

        printf("\n After sorting : \n");
        for(int i=0;i<n;i++) {
                printf("%d  ",arr[i]);
        }
       printf("\n...........................................................\n");
         
       int childPids[n];// For storing processIds of child process

        for(int i=0;i<n;i++) {
                pid_t p = fork();

                if(p < 0) {
                        perror("fork failed: ");
                        exit(1);
                } else if (p == 0) {

                        sort(arr,n);
                        printf("%dth largest number is %d and processId: %d\n",i,arr[i],getpid());
                        exit(arr[i]);
                } else {

			childPids[i] = p;
                }
	}

	int result[n];

	for(int i=0;i<n;i++) {
		int status;
		pid_t exitPid = waitpid(childPids[i], &status, 0);//wait for a particular child process specified by processId of child process
                
		if(exitPid == -1) {
			perror("Waitpid failed: ");
			exit(1);
		}

		if(WIFEXITED(status)) {
			result[i] = WEXITSTATUS(status);//retrieve the status returned by child process
		}
	}

	printf("\n printing exit values from parent process: \n");
	for(int i=0;i<n;i++) {
		printf("%d \n",result[i]);
	}
	return 0;
}

