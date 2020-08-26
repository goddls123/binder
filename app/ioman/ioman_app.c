
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include "binder_ioctl.h"

int main(){
	int fd;
	int a = PT_MONITOR;
	//char *buffer;
	//char buffer[16] = {0,};	// 
	char buffer[16];	// 
	struct io d;

	fd = open("/dev/binder",O_RDWR);
	printf("fd = %d\n", fd);

	if(fd<0){
		perror("/dev/binder\n");
		exit(-1);
	}	
	else
		printf("binder detect\n");

	ioctl(fd, IOSET_TYPE, PT_MONITOR);
	printf("send: ping\n");
	ioctl(fd, IO_PING, buffer);


	printf("recv: %s\n",buffer);
	while(1){}

	close(fd);	
	return 0;
}
