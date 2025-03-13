/*Write a complete C program that reads for an array of n numbers (value of n is given by the user). 
The program then creates n child processes P1, P2, ..., Pn such that Pi, 1 ≤  i ≤  n, 
computes and prints the ith largest number from the array.*/
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>

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

	for(int i=0;i<n;i++) {
		pid_t p = fork();

		if(p < 0) {
			perror("fork failed: ");
			exit(1);
		} else if (p == 0) {

		        sort(arr,n);
			printf("%dth largest number is %d and processId: %d\n",i,arr[i],getpid());
			exit(0);
		}
	}
	return 0;
}
