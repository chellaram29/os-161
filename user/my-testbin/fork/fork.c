/*
 * An example program.
 */
#include <unistd.h>
#include<stdio.h>
int main(){

	int pid;
	pid=fork();

	printf("pid:%d\n",pid);


return 0;

}
