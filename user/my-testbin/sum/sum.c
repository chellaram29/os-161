/*
 * sum.c
 *
 *  Created on: Dec 21, 2015
 *      Author: trinity
 */

#include<stdio.h>
#include<stdlib.h>
int main(int argc, char *argv[]){


for(int i=0;i<argc;i++){

	printf("addr of %d th arg:%ld\n",i,(unsigned long)argv[i]);
	printf("val of %d th arg:%s\n",i,argv[i]);
}

	(void)argc;
	(void)argv;

}



