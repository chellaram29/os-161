/*
 * An example program.
 */
#include <unistd.h>
#include<stdio.h>
#include<stdlib.h>
int main(){

	int pid;
	pid=fork();
	if(pid==0){

		printf("child process");
		exit(93);

	}
	else{
		int status;
		waitpid(pid,&status,0);
		printf("status is:%d\n",status);
		printf("paretn process\n");


	}


return 0;

}
