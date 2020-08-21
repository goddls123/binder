
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>


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

	while(1){}

	close(fd);	
	return 0;
}
