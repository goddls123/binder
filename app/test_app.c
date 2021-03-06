
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include "binder_ioctl.h"

int main(){
	int fd;
	
	fd = open("/dev/binder",O_RDWR);
	printf("fd = %d\n", fd);

	if(fd<0){
		perror("/dev/binder\n");
		exit(-1);
	}	
	else
		printf("binder detect\n");


	ioctl(fd,IOSET_TYPE,PT_IOMAN);
	while(1){}

	close(fd);	
	return 0;
}
