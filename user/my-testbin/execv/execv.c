/*
 * execv.c
 *
 *  Created on: Dec 21, 2015
 *      Author: trinity
 */


#include<stdio.h>
#include<unistd.h>
int main(){

char *argv[3];

argv[0]=(char *)"ramalingam";
argv[1]=(char *)"kalyanasundaram ramalingam";
argv[2]=NULL;


execv("sum",argv);



return 0;

}
