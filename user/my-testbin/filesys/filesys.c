/*
 * An example program.
 */
#include <unistd.h>
#include<stdio.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<kern/seek.h>
#include<stdio.h>

int
main()
{
	int fd=open("one.txt",O_WRONLY);
	char buf[50]="I love Kernel Programming\n";
	char buf1[50]="Ram is a gud boy\n";
	off_t rv;
	int byteswritten=0;
	extern int errno;
	printf("return value:%d, errno :%d\n",fd,errno);

	byteswritten=write(fd,buf,strlen(buf));
	printf("byteswritten=%d",byteswritten);
 rv=lseek(fd,1000,SEEK_SET);
 	 printf("lseek rv:%lld\n",rv);
	byteswritten=write(fd,buf1,strlen(buf1));
	printf("byteswritten=%d",byteswritten);

	return 0; /* avoid compiler warnings */
}
