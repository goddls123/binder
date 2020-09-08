
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "binder_ioctl.h"
#include <string.h>

int main()
{
	int fd;
	int a = PT_MONITOR;
	char buffer[16] = {0,};	 
	int r;

	fd = open("/dev/binder",O_RDWR);
	printf("fd = %d\n", fd);

	if(fd<0){
		perror("/dev/binder\n");
		exit(-1);
	}	
	else
		printf("osd\n");

	ioctl(fd, IOSET_TYPE, PT_OSD);
	
	while(1){
		if(( r=ioctl(fd, DM_DISPLAY_TEXT, buffer))>0){
			read(fd,buffer, sizeof(char)*15);
			printf("%s\n",buffer);
		}
		sleep(0.5);
	}

	close(fd);	
	return 0;
}
