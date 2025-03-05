#include<stdio.h>
#include<unistd.h>

int main() {
	pid_t processId;
	printf("Hello World 1\n");
	processId = fork();

	if(processId == 0) {

		printf("Child process\n");
	} else {
		printf("Parent process!\n");
	}
	printf("Hello World 2\n");
}
