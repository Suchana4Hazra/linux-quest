#include<stdio.h>
#include<unistd.h>

int main() {
	pid_t p;

	p = fork();

	if(p == 0) {
		int num1,num2;
		printf("Child process created\n");
		printf("Enter two numbers to add\n");
		scanf("%d %d",&num1,&num2);
		int ans = num1+num2;
		printf("%d+%d = %d",num1,num2,ans);
	} else {
		int num;
		printf("Parent process\n");
		printf("Enter any number to check whether it is even or odd\n");
		scanf("%d",&num);
		if(num % 2 == 0) {
			printf("%d is even\n",num);
		}else {
			printf("%d is odd\n",num);
		}
	}
	int num3,num4;
	printf("MUltiplication\n");
	printf("Enter two numbers: \n");
	scanf("%d %d",&num3,&num4);
	int ans = num3*num4;
	printf("%d x %d = %d",num3,num4,ans);
}



		

