#include<stdio.h>
#include<unistd.h>

int main() {
	//Executed by parent process only
	printf("Hello World 1\n");
	fork();
	//executed by both parent and child processes
	printf("Hello World 2!\n");
}
